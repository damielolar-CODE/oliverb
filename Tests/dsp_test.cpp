// =============================================================================
//  OLIVERB — DSP test harness
//
//  Builds and runs the entire DSP core with no JUCE and no plugin host, so the
//  maths can be checked in isolation:
//
//    1. the high-pass really is ~18 dB/octave
//    2. IMPEDANCE really produces a corner peak
//    3. nothing blows up or goes NaN under abusive settings
//    4. the echo self-oscillates without exploding
//    5. the spring tank actually rings and then decays
//
//  Build & run:  cmake --build build --target dsp_test && ./build/dsp_test
//           or:  g++ -O2 -std=c++17 Tests/dsp_test.cpp -o dsp_test && ./dsp_test
// =============================================================================

#include "../Source/dsp/PassiveHighPass.h"
#include "../Source/dsp/TapeEcho.h"
#include "../Source/dsp/SpringReverb.h"
#include "../Source/dsp/Modulation.h"
#include "../Source/dsp/ValveStage.h"

#include <cmath>
#include <cstdio>
#include <string>

namespace
{
constexpr double kPi = 3.14159265358979323846; // MSVC's <cmath> has no M_PI without _USE_MATH_DEFINES
constexpr double kSR = 48000.0;
int failures = 0;

void check (bool condition, const std::string& what)
{
    std::printf ("  [%s] %s\n", condition ? " ok " : "FAIL", what.c_str());
    if (! condition) ++failures;
}

bool finiteBlock (const float* d, int n, float bound)
{
    for (int i = 0; i < n; ++i)
        if (! std::isfinite (d[i]) || std::fabs (d[i]) > bound)
            return false;
    return true;
}

/** Steady-state magnitude response, in dB, measured by correlating against a
    sine at the test frequency after the filter has settled. */
float magnitudeDb (wh::PassiveHighPass& f, float hz)
{
    const int settle = static_cast<int> (kSR * 0.5);
    const int measure = static_cast<int> (kSR * 0.5);
    const double w = 2.0 * kPi * hz / kSR;

    for (int i = 0; i < settle; ++i)
        f.process (static_cast<float> (std::sin (w * i)));

    double re = 0.0, im = 0.0;
    for (int i = 0; i < measure; ++i)
    {
        const double phase = w * (settle + i);
        const float y = f.process (static_cast<float> (std::sin (phase)));
        re += y * std::cos (phase);
        im += y * std::sin (phase);
    }

    const double mag = 2.0 * std::sqrt (re * re + im * im) / measure;
    return 20.0f * static_cast<float> (std::log10 (std::max (1.0e-9, mag)));
}
} // namespace

int main()
{
    std::printf ("\n=== OLIVERB DSP core ===\n\n");

    // -------------------------------------------------------------------------
    std::printf ("Filter: switch positions\n");
    for (int s = 0; s < wh::PassiveHighPass::kNumSteps; ++s)
        std::printf ("  step %2d :  bank A %7.1f Hz   bank B %7.1f Hz\n",
                     s + 1,
                     wh::PassiveHighPass::stepFrequency (0, s),
                     wh::PassiveHighPass::stepFrequency (1, s));

    // -------------------------------------------------------------------------
    std::printf ("\nFilter: slope (linear settings, corner = 500 Hz)\n");
    {
        auto make = [] (wh::PassiveHighPass& f)
        {
            f.prepare (kSR);
            f.setStep (0, 5);            // 500 Hz, bank A
            f.setImpedance (0.0f);
            f.setMagnetism (0.0f);
            f.setCharacter (0.0f);
            f.setArtefacts (0.0f);
            f.setDynamics (0.0f);
            f.setGain (0.0f);
        };

        wh::PassiveHighPass a, b, c, d;
        make (a); make (b); make (c); make (d);

        const float m125  = magnitudeDb (a, 125.0f);
        const float m250  = magnitudeDb (b, 250.0f);
        const float m500  = magnitudeDb (c, 500.0f);
        const float m4k   = magnitudeDb (d, 4000.0f);

        std::printf ("   125 Hz: %7.2f dB\n   250 Hz: %7.2f dB\n   500 Hz: %7.2f dB\n  4000 Hz: %7.2f dB\n",
                     m125, m250, m500, m4k);

        const float octave = m250 - m125;   // one octave of stopband
        std::printf ("  stopband slope: %.2f dB/octave (target ~18)\n", octave);

        check (octave > 15.0f && octave < 21.0f, "slope is third-order (18 dB/oct)");
        check (m4k > -1.5f && m4k < 1.0f, "passband is flat well above the corner");
        check (m500 < -1.0f && m500 > -8.0f, "corner sits near the switch position");
    }

    // -------------------------------------------------------------------------
    std::printf ("\nFilter: impedance produces a corner peak\n");
    {
        wh::PassiveHighPass flat, peaked;
        for (auto* f : { &flat, &peaked })
        {
            f->prepare (kSR);
            f->setStep (0, 5);
            f->setMagnetism (0.0f);
            f->setCharacter (0.0f);
            f->setArtefacts (0.0f);
            f->setGain (0.0f);
        }
        flat.setImpedance (0.0f);
        peaked.setImpedance (1.0f);

        const float a = magnitudeDb (flat, 620.0f);
        const float b = magnitudeDb (peaked, 620.0f);
        std::printf ("  620 Hz  terminated: %6.2f dB   open: %6.2f dB   lift: %.2f dB\n",
                     a, b, b - a);
        check (b - a > 3.0f, "open termination lifts the corner");
    }

    // -------------------------------------------------------------------------
    std::printf ("\nFilter: abusive settings stay finite\n");
    {
        wh::PassiveHighPass f;
        f.prepare (kSR);
        f.setImpedance (1.0f);
        f.setMagnetism (1.0f);
        f.setCharacter (1.0f);
        f.setDynamics (1.0f);
        f.setArtefacts (1.0f);
        f.setGain (18.0f);

        wh::Noise n;
        std::vector<float> out (4096);
        bool ok = true;
        float peak = 0.0f;

        for (int block = 0; block < 200; ++block)
        {
            f.setStep (block % 2, block % wh::PassiveHighPass::kNumSteps);  // hammer the switch
            for (auto& s : out) s = f.process (n.next() * 2.0f);
            ok = ok && finiteBlock (out.data(), (int) out.size(), 100.0f);
            for (auto s : out) peak = std::max (peak, std::fabs (s));
        }
        std::printf ("  peak: %.3f\n", peak);
        check (ok, "no NaN/inf while sweeping the switch under heavy drive");
    }

    // -------------------------------------------------------------------------
    std::printf ("\nFilter: audio-rate corner modulation stays finite\n");
    {
        wh::PassiveHighPass f;
        f.prepare (kSR);
        f.setStep (0, 6);
        f.setImpedance (0.9f);
        f.setMagnetism (0.6f);
        f.setCharacter (0.5f);
        f.setGain (6.0f);

        wh::LFO lfo;
        lfo.prepare (kSR);
        lfo.setRateHz (20.0f);

        wh::Noise n;
        bool ok = true;
        float peak = 0.0f;

        for (int shape = 0; shape < wh::LFO::NumShapes && ok; ++shape)
        {
            lfo.setShape (shape);
            for (int i = 0; i < static_cast<int> (kSR); ++i)
            {
                f.setModOctaves (lfo.next() * 3.0f);        // full +/-3 octave throw
                const float y = f.process (n.next());
                if (! std::isfinite (y)) { ok = false; break; }
                peak = std::max (peak, std::fabs (y));
            }
        }
        std::printf ("  peak across all LFO shapes at +/-3 oct: %.3f\n", peak);
        check (ok, "modulated filter never goes non-finite");
        check (peak < 50.0f, "modulated filter stays bounded");
    }

    // -------------------------------------------------------------------------
    for (float valveAmp : { 0.0f, 1.0f })
    {
        std::printf ("\nEcho: runaway feedback is contained (%s record amp)\n",
                     valveAmp > 0.0f ? "valve" : "solid-state");
        wh::TapeEcho e;
        e.prepare (kSR);
        e.setValve (valveAmp);
        e.setFeedback (1.25f);
        e.setInputLevel (1.5f);
        e.setOutputLevel (1.0f);
        e.setMix (1.0f);
        e.setHiss (1.0f);
        e.setAge (1.0f);
        e.snapTimeMs (250.0f);

        float peak = 0.0f;
        bool ok = true;

        for (int i = 0; i < static_cast<int> (kSR * 20); ++i)
        {
            float l = (i < 4800) ? std::sin (2.0f * wh::kPi * 220.0f * i / (float) kSR) : 0.0f;
            float r = l;
            if (i == static_cast<int> (kSR * 5)) e.setSend (false);   // close the door
            if (i == static_cast<int> (kSR * 10)) e.setTimeMs (700.0f); // drag the transport
            e.process (l, r);
            if (! std::isfinite (l) || ! std::isfinite (r)) { ok = false; break; }
            peak = std::max (peak, std::max (std::fabs (l), std::fabs (r)));
        }
        std::printf ("  peak after 20 s at feedback 1.25: %.3f\n", peak);
        check (ok, "self-oscillation never goes non-finite");
        check (peak < 12.0f, "self-oscillation limits into the record amp");
        check (peak > 0.05f, "the echo actually sustains");
    }

    // -------------------------------------------------------------------------
    std::printf ("\nValve: unity at zero drive, bit-exact at mix 0\n");
    {
        wh::ValveStage v;
        v.prepare (kSR);
        v.setDrive (0.0f);
        v.setBias (0.5f);
        v.setSag (0.0f);
        v.setTone (0.0f);
        v.setMix (1.0f);

        double inE = 0.0, outE = 0.0;
        for (int i = 0; i < static_cast<int> (kSR); ++i)
        {
            const float x = 0.3f * std::sin (2.0f * wh::kPi * 220.0f * i / (float) kSR);
            float l = x, r = x;
            v.process (l, r);
            if (i > static_cast<int> (kSR * 0.25))
            {
                inE  += x * x;
                outE += l * l;
            }
        }
        const float gainDb = 10.0f * static_cast<float> (std::log10 (outE / inE));
        std::printf ("  gain at zero drive: %.2f dB\n", gainDb);
        check (std::fabs (gainDb) < 1.0f, "zero drive is within 1 dB of unity");

        v.setMix (0.0f);
        v.setDrive (1.0f);
        bool exact = true;
        for (int i = 0; i < 4096; ++i)
        {
            const float x = std::sin (0.01f * i);
            float l = x, r = x;
            v.process (l, r);
            if (l != x || r != x) { exact = false; break; }
        }
        check (exact, "mix 0 passes the dry signal untouched");
    }

    // -------------------------------------------------------------------------
    std::printf ("\nValve: bias produces even harmonics\n");
    {
        auto secondHarmonicDb = [] (float bias, float drive)
        {
            wh::ValveStage v;
            v.prepare (kSR);
            v.setDrive (drive);
            v.setBias (bias);
            v.setSag (0.0f);
            v.setTone (0.0f);
            v.setMix (1.0f);

            const double w = 2.0 * kPi * 220.0 / kSR;   // 220 cycles per second: leakage-free
            const int settle = static_cast<int> (kSR * 0.5);
            const int measure = static_cast<int> (kSR);

            for (int i = 0; i < settle; ++i)
            {
                float l = 0.5f * static_cast<float> (std::sin (w * i)), r = l;
                v.process (l, r);
            }

            double re1 = 0.0, im1 = 0.0, re2 = 0.0, im2 = 0.0;
            for (int i = 0; i < measure; ++i)
            {
                const double ph = w * (settle + i);
                float l = 0.5f * static_cast<float> (std::sin (ph)), r = l;
                v.process (l, r);
                re1 += l * std::cos (ph);       im1 += l * std::sin (ph);
                re2 += l * std::cos (2.0 * ph); im2 += l * std::sin (2.0 * ph);
            }
            const double m1 = std::sqrt (re1 * re1 + im1 * im1);
            const double m2 = std::sqrt (re2 * re2 + im2 * im2);
            return 20.0f * static_cast<float> (std::log10 (std::max (1.0e-12, m2 / m1)));
        };

        const float clean = secondHarmonicDb (0.0f, 0.0f);
        const float cold  = secondHarmonicDb (0.0f, 0.5f);
        const float hot   = secondHarmonicDb (1.0f, 0.5f);
        std::printf ("  2nd harmonic vs fundamental   clean: %6.1f dB   cold bias: %6.1f dB   hot bias: %6.1f dB\n",
                     clean, cold, hot);
        check (clean < -36.0f, "zero drive is essentially clean (under ~1.5% 2nd harmonic)");
        check (hot > -30.0f, "hot bias puts the 2nd harmonic within 30 dB of the fundamental");
        check (hot > cold + 3.0f, "hotter bias means more even-order content");
    }

    // -------------------------------------------------------------------------
    std::printf ("\nValve: abusive settings stay finite\n");
    {
        wh::ValveStage v;
        v.prepare (kSR);
        v.setDrive (1.0f);
        v.setBias (1.0f);
        v.setSag (1.0f);
        v.setTone (1.0f);
        v.setMix (1.0f);

        wh::Noise n;
        bool ok = true;
        float peak = 0.0f;
        for (int i = 0; i < static_cast<int> (kSR * 5); ++i)
        {
            // Noise, DC offset and a square wave at +20 dB: nothing a real input should be.
            float l = n.next() * 10.0f + 3.0f + ((i / 200) % 2 ? 8.0f : -8.0f);
            float r = -l;
            v.process (l, r);
            if (! std::isfinite (l) || ! std::isfinite (r)) { ok = false; break; }
            peak = std::max (peak, std::max (std::fabs (l), std::fabs (r)));
        }
        std::printf ("  peak: %.3f   lamp: %.2f\n", peak, v.glowLevel());
        check (ok, "valve never goes non-finite");
        check (peak < 8.0f, "valve output stays bounded");
        check (v.glowLevel() > 0.2f, "the lamp lights when the valve is driven");
    }

    // -------------------------------------------------------------------------
    std::printf ("\nSpring: rings, then decays\n");
    {
        wh::SpringReverb s;
        s.prepare (kSR);
        s.setAmount (1.0f);
        s.setDecay (0.8f);
        s.setDrive (0.5f);

        double early = 0.0, late = 0.0, tail = 0.0;
        bool ok = true;

        for (int i = 0; i < static_cast<int> (kSR * 8); ++i)
        {
            float l = (i == 0) ? 1.0f : 0.0f;
            float r = l;
            s.process (l, r);
            if (! std::isfinite (l)) { ok = false; break; }

            const double e = l * l;
            if (i > 2000 && i < 12000)                     early += e;
            else if (i > 40000 && i < 50000)               late  += e;
            else if (i > static_cast<int> (kSR * 7))       tail  += e;
        }
        std::printf ("  energy  early: %.3e   late: %.3e   +7 s: %.3e\n", early, late, tail);
        check (ok, "spring stays finite");
        check (early > 1.0e-6, "impulse actually excites the tank");
        check (late < early, "tank decays rather than grows");
        check (tail < late * 0.5, "tail dies away");
    }

    // -------------------------------------------------------------------------
    std::printf ("\n%s  (%d failure%s)\n\n",
                 failures == 0 ? "ALL CHECKS PASSED" : "CHECKS FAILED",
                 failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
