// =============================================================================
//  Waterhouse — Utils.h
//  Small, dependency-free DSP primitives shared by every module.
//  Pure C++17: no JUCE here, so the whole DSP core can be unit-tested
//  from a plain command-line harness (see Tests/dsp_test.cpp).
// =============================================================================
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace wh
{

// -----------------------------------------------------------------------------
//  Constants + tiny helpers
// -----------------------------------------------------------------------------
constexpr float kPi = 3.14159265358979323846f;

inline float clampf (float v, float lo, float hi) noexcept
{
    return v < lo ? lo : (v > hi ? hi : v);
}

inline float lerp (float a, float b, float t) noexcept { return a + (b - a) * t; }

/** Flush denormals. Cheap insurance for long feedback tails. */
inline float fixDenorm (float v) noexcept
{
    return (std::fabs (v) < 1.0e-20f) ? 0.0f : v;
}

/** Fast-ish odd-symmetric soft clipper. Continuous first derivative,
    bounded by +/-1, and noticeably cheaper than std::tanh in a feedback path. */
inline float softClip (float x) noexcept
{
    x = clampf (x, -3.0f, 3.0f);
    return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
}

/** Asymmetric drive stage. Even-order content comes from the bias term,
    which is what gives valve/transformer-ish "thickness" rather than the
    purely odd character of a symmetric clipper. */
inline float asymDrive (float x, float drive, float asym) noexcept
{
    const float b = asym * 0.35f;
    return (softClip ((x + b) * drive) - softClip (b * drive)) / drive;
}

/** dB -> linear gain. */
inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }

// -----------------------------------------------------------------------------
//  Deterministic white-noise source (xorshift32)
// -----------------------------------------------------------------------------
class Noise
{
public:
    explicit Noise (uint32_t seed = 0x1234567u) : state (seed | 1u) {}

    /** Uniform in [-1, 1). */
    float next() noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return static_cast<float> (static_cast<int32_t> (state)) * 4.6566129e-10f;
    }

    void reset (uint32_t seed) noexcept { state = seed | 1u; }

private:
    uint32_t state;
};

// -----------------------------------------------------------------------------
//  Exponential parameter smoother (one-pole)
// -----------------------------------------------------------------------------
class Smoother
{
public:
    void prepare (double sampleRate, float milliseconds) noexcept
    {
        fs = static_cast<float> (sampleRate);
        setTime (milliseconds);
    }

    void setTime (float milliseconds) noexcept
    {
        const float t = std::max (0.01f, milliseconds) * 0.001f;
        coeff = 1.0f - std::exp (-1.0f / (t * fs));
    }

    void setTarget (float v) noexcept { target = v; }
    void snap (float v) noexcept { target = current = v; }

    float next() noexcept
    {
        current += coeff * (target - current);
        return current;
    }

    float value() const noexcept { return current; }

private:
    float fs = 44100.0f, coeff = 0.01f, current = 0.0f, target = 0.0f;
};

// -----------------------------------------------------------------------------
//  TPT one-pole (Zavalishin). Zero-delay-feedback topology: stable and
//  well behaved when the cutoff is modulated at audio rate, which matters a
//  lot for the non-linear inductor model in PassiveHighPass.
// -----------------------------------------------------------------------------
class OnePole
{
public:
    void prepare (double sampleRate) noexcept
    {
        fs = static_cast<float> (sampleRate);
        reset();
        setCutoff (1000.0f);
    }

    void reset() noexcept { z = 0.0f; }

    void setCutoff (float hz) noexcept
    {
        hz = clampf (hz, 5.0f, fs * 0.45f);
        const float g = std::tan (kPi * hz / fs);
        G = g / (1.0f + g);
    }

    /** Set the integrator gain directly (skips the tan() call). */
    void setG (float g) noexcept { G = g / (1.0f + g); }

    float lowpass (float x) noexcept
    {
        const float v = (x - z) * G;
        const float lp = v + z;
        z = fixDenorm (lp + v);
        return lp;
    }

    float highpass (float x) noexcept { return x - lowpass (x); }

private:
    float fs = 44100.0f, G = 0.1f, z = 0.0f;
};

// -----------------------------------------------------------------------------
//  DC blocker
// -----------------------------------------------------------------------------
class DCBlocker
{
public:
    void prepare (double sampleRate) noexcept
    {
        R = 1.0f - (2.0f * kPi * 8.0f / static_cast<float> (sampleRate));
        reset();
    }

    void reset() noexcept { x1 = y1 = 0.0f; }

    float process (float x) noexcept
    {
        const float y = x - x1 + R * y1;
        x1 = x;
        y1 = fixDenorm (y);
        return y;
    }

private:
    float R = 0.999f, x1 = 0.0f, y1 = 0.0f;
};

// -----------------------------------------------------------------------------
//  Envelope follower with separate attack/release
// -----------------------------------------------------------------------------
class EnvFollower
{
public:
    void prepare (double sampleRate) noexcept
    {
        fs = static_cast<float> (sampleRate);
        setTimes (5.0f, 80.0f);
        env = 0.0f;
    }

    void setTimes (float attackMs, float releaseMs) noexcept
    {
        aCoeff = 1.0f - std::exp (-1.0f / (std::max (0.05f, attackMs) * 0.001f * fs));
        rCoeff = 1.0f - std::exp (-1.0f / (std::max (0.05f, releaseMs) * 0.001f * fs));
    }

    float process (float x) noexcept
    {
        const float r = std::fabs (x);
        env += (r > env ? aCoeff : rCoeff) * (r - env);
        return fixDenorm (env);
    }

    float value() const noexcept { return env; }

private:
    float fs = 44100.0f, aCoeff = 0.1f, rCoeff = 0.01f, env = 0.0f;
};

// -----------------------------------------------------------------------------
//  Fractional delay line with 3rd-order Lagrange interpolation.
//  Lagrange (rather than plain linear) keeps the high end intact while the
//  read head is being modulated by wow and flutter.
// -----------------------------------------------------------------------------
class DelayLine
{
public:
    void prepare (double sampleRate, float maxDelaySeconds)
    {
        const int n = static_cast<int> (sampleRate * maxDelaySeconds) + 8;
        buffer.assign (static_cast<size_t> (n), 0.0f);
        size = n;
        writeIdx = 0;
    }

    void reset() { std::fill (buffer.begin(), buffer.end(), 0.0f); writeIdx = 0; }

    void write (float x) noexcept
    {
        buffer[static_cast<size_t> (writeIdx)] = x;
        if (++writeIdx >= size) writeIdx = 0;
    }

    /** Read `delaySamples` behind the write head. */
    float read (float delaySamples) const noexcept
    {
        delaySamples = clampf (delaySamples, 1.0f, static_cast<float> (size - 4));

        const float readPos  = static_cast<float> (writeIdx) - delaySamples;
        const int   i0       = static_cast<int> (std::floor (readPos));
        const float frac     = readPos - static_cast<float> (i0);

        const float ym1 = at (i0 - 1);
        const float y0  = at (i0);
        const float y1  = at (i0 + 1);
        const float y2  = at (i0 + 2);

        // 3rd-order Lagrange
        const float c0 = y0;
        const float c1 = 0.5f * (y1 - ym1);
        const float c2 = ym1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
        const float c3 = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);
        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

    int length() const noexcept { return size; }

private:
    float at (int idx) const noexcept
    {
        idx %= size;
        if (idx < 0) idx += size;
        return buffer[static_cast<size_t> (idx)];
    }

    std::vector<float> buffer;
    int size = 0, writeIdx = 0;
};

// -----------------------------------------------------------------------------
//  Schroeder allpass with a fixed integer delay — the building block of the
//  spring model's dispersive chain.
// -----------------------------------------------------------------------------
class Allpass
{
public:
    void prepare (int delaySamples, float coefficient)
    {
        buffer.assign (static_cast<size_t> (std::max (1, delaySamples)), 0.0f);
        idx = 0;
        g = coefficient;
    }

    void reset() { std::fill (buffer.begin(), buffer.end(), 0.0f); idx = 0; }
    void setCoefficient (float coefficient) noexcept { g = coefficient; }

    float process (float x) noexcept
    {
        const float delayed = buffer[static_cast<size_t> (idx)];
        const float v = x + g * delayed;
        buffer[static_cast<size_t> (idx)] = fixDenorm (v);
        if (++idx >= static_cast<int> (buffer.size())) idx = 0;
        return delayed - g * v;
    }

private:
    std::vector<float> buffer;
    int idx = 0;
    float g = 0.6f;
};

// -----------------------------------------------------------------------------
//  State-variable bandpass/peaking helper (TPT/Cytomic form), used for
//  fixed voicing filters (head bump, spring body).
// -----------------------------------------------------------------------------
class SVF
{
public:
    void prepare (double sampleRate) noexcept
    {
        fs = static_cast<float> (sampleRate);
        reset();
        set (1000.0f, 0.707f);
    }

    void reset() noexcept { ic1 = ic2 = 0.0f; }

    void set (float hz, float Q) noexcept
    {
        hz = clampf (hz, 10.0f, fs * 0.45f);
        const float g = std::tan (kPi * hz / fs);
        k  = 1.0f / std::max (0.05f, Q);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    void process (float x, float& lp, float& bp, float& hp) noexcept
    {
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = fixDenorm (2.0f * v1 - ic1);
        ic2 = fixDenorm (2.0f * v2 - ic2);
        lp = v2;
        bp = v1;
        hp = x - k * v1 - v2;
    }

    float bandpass (float x) noexcept
    {
        float lp, bp, hp;
        process (x, lp, bp, hp);
        return bp;
    }

    float lowpass (float x) noexcept
    {
        float lp, bp, hp;
        process (x, lp, bp, hp);
        return lp;
    }

private:
    float fs = 44100.0f;
    float k = 1.0f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
    float ic1 = 0.0f, ic2 = 0.0f;
};

} // namespace wh
