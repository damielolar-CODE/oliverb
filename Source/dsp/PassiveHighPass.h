// =============================================================================
//  OLIVERB — PassiveHighPass.h
//
//  A model of the passive, inductor-based, stepped variable high-pass filter
//  of the kind used on 1960s broadcast/console EQ modules (the Altec 9069B
//  being the famous example — the "big knob" bolted to the top right of King
//  Tubby's MCI desk).
//
//  What the real circuit is, and how that maps onto this code:
//
//   * It is a passive constant-k / T-network ladder: series capacitors with
//     shunt inductors. Three reactive elements => 3rd order => 18 dB/octave.
//     Here: one resonant 2nd-order section (SVF) + one 1st-order section,
//     tuned to the same corner. Same slope, same corner, far cheaper.
//
//   * The peak at the corner is not "resonance" in the synth sense — it is
//     what happens when the network is not terminated in its design impedance
//     (600 ohm). Under-terminate it and the corner lifts by several dB;
//     terminate it properly and it is close to maximally flat. That is what
//     IMPEDANCE does here: it sets Q, nothing else.
//
//   * The shunt inductors are wound on a real core. Push level into them and
//     the core partially saturates: inductance falls, so the corner frequency
//     climbs slightly with signal level, and odd/even harmonics appear.
//     MAGNETISM scales that level-dependent shift; DYNAMICS sets how fast the
//     core "follows" the signal (slow = a gentle breathing lift, fast = it
//     tracks the envelope and starts to behave like an auto-wah).
//
//   * CHARACTER drives the network's internal feedback path into soft
//     asymmetric clipping — the harmonic content of a loaded passive network,
//     independent of the core model.
//
//   * A real rotary switch shorts and re-connects capacitors while audio is
//     passing. That makes a thump. ARTEFACTS controls how much of that click
//     is reproduced (at 0 the corner glides smoothly instead, which the
//     hardware cannot do).
//
//  The frequency tables below are a plausible reconstruction of an 11-position
//  70 Hz - 7.5 kHz switch: they are not measured from a specific unit.
// =============================================================================
#pragma once

#include "Utils.h"

namespace wh
{

class PassiveHighPass
{
public:
    static constexpr int kNumSteps = 11;

    /** Two switchable capacitor banks, as per the front-panel TYPE switch.
        A = the classic broad sweep. B = a tighter set biased to the low end,
        useful when the filter is doing musical work rather than effects. */
    static float stepFrequency (int type, int step) noexcept
    {
        static const float bankA[kNumSteps] =
            { 70.0f, 100.0f, 150.0f, 220.0f, 330.0f, 500.0f,
              750.0f, 1100.0f, 1800.0f, 3500.0f, 7500.0f };

        static const float bankB[kNumSteps] =
            { 50.0f, 80.0f, 120.0f, 170.0f, 240.0f, 350.0f,
              520.0f, 800.0f, 1300.0f, 2400.0f, 5000.0f };

        step = std::max (0, std::min (kNumSteps - 1, step));
        return (type == 0 ? bankA : bankB)[step];
    }

    // -------------------------------------------------------------------------
    void prepare (double sampleRate)
    {
        fs = static_cast<float> (sampleRate);
        env.prepare (sampleRate);
        clickFilter.prepare (sampleRate);
        clickFilter.set (180.0f, 0.9f);
        gSmooth.prepare (sampleRate, 4.0f);
        reset();

        setStep (0, 0);
        gSmooth.snap (gTarget);
        updateDynamics();
    }

    void reset() noexcept
    {
        ic1 = ic2 = 0.0f;
        z1 = 0.0f;
        env.prepare (fs);
        clickFilter.reset();
        clickEnergy = 0.0f;
    }

    // ---- Parameters ---------------------------------------------------------

    /** Selects the switch position. Re-selecting the same position is a no-op,
        so this is safe to call every block from the audio thread. */
    void setStep (int type, int step) noexcept
    {
        if (type == curType && step == curStep) return;

        const bool firstTime = (curStep < 0);
        curType = type;
        curStep = step;

        const float hz = clampf (stepFrequency (type, step), 20.0f, fs * 0.45f);
        gTarget = std::tan (kPi * hz / fs);

        if (firstTime)
            gSmooth.snap (gTarget);
        else
            pendingClick = true;      // the switch was thrown while audio was live
    }

    /** 0 = correctly terminated (flat), 1 = wide open (strong corner peak). */
    void setImpedance (float v) noexcept
    {
        impedance = clampf (v, 0.0f, 1.0f);
        const float Q = lerp (0.62f, 3.2f, impedance * impedance * 0.6f + impedance * 0.4f);
        k = 1.0f / Q;
    }

    void setMagnetism (float v) noexcept { magnetism = clampf (v, 0.0f, 1.0f); }
    void setCharacter (float v) noexcept { character = clampf (v, 0.0f, 1.0f); }
    void setArtefacts (float v) noexcept { artefacts = clampf (v, 0.0f, 1.0f); }

    /** 0 = slow, sluggish core (a gentle lift under load).
        1 = fast core (tracks the envelope; behaves like an envelope filter). */
    void setDynamics (float v) noexcept
    {
        dynamics = clampf (v, 0.0f, 1.0f);
        updateDynamics();
    }

    /** Internal make-up / drive trim, in dB. */
    void setGain (float db) noexcept { gain = dbToGain (clampf (db, -18.0f, 18.0f)); }

    /** Per-sample corner modulation, in octaves relative to the switch
        position. Fed by the LFO and envelope follower; safe at audio rate
        because the TPT topology tolerates fast cutoff changes. */
    void setModOctaves (float oct) noexcept { modOct = clampf (oct, -5.0f, 5.0f); }

    // ---- Audio --------------------------------------------------------------
    float process (float x) noexcept
    {
        // --- rotary switch transient -----------------------------------------
        if (pendingClick)
        {
            pendingClick = false;
            // A real switch thump scales with how much signal is sitting on the
            // caps at the moment of the change, plus a fixed contact component.
            clickEnergy = artefacts * (0.35f + 1.4f * env.value());
            // Hard-jump the corner when artefacts are up; glide when they're not.
            gSmooth.setTime (lerp (25.0f, 0.05f, artefacts));
        }
        gSmooth.setTarget (gTarget);
        float gBase = gSmooth.next();

        // Modulation scales g as 2^octaves. Exact for small g, slightly flat at
        // the very top of the range — the same trade the core model makes, and
        // it keeps a transcendental out of the per-sample path.
        if (std::fabs (modOct) > 1.0e-6f)
            gBase = clampf (gBase * std::exp2 (modOct), 0.0008f, 2.6f);

        float in = x * gain;

        if (clickEnergy > 1.0e-4f)
        {
            in += clickFilter.bandpass (clickEnergy) * 2.0f;
            clickEnergy *= 0.9992f;
        }

        // --- core saturation: L falls as current rises, so the corner lifts ---
        const float e = env.process (ic1);          // ic1 tracks inductor current
        const float lift = 1.0f + magnetism * (2.6f * e) / (1.0f + 1.8f * e);
        float g = clampf (gBase * lift, gBase * 0.5f, std::min (gBase * 6.0f, 2.6f));

        // Losing inductance also costs a little Q — a saturating core is a lossy core.
        const float kEff = k * (1.0f + magnetism * e * 0.8f);

        // --- 2nd-order section (TPT SVF, non-linear feedback path) ------------
        const float a1 = 1.0f / (1.0f + g * (g + kEff));
        const float a2 = g * a1;
        const float a3 = g * a2;

        const float v3 = in - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = fixDenorm (2.0f * v1 - ic1);
        ic2 = fixDenorm (2.0f * v2 - ic2);

        // CHARACTER: the damping path is where a loaded passive network
        // generates its harmonics. Blend clean -> asymmetrically driven.
        float damped = kEff * v1;
        if (character > 0.0001f)
        {
            const float drive = 1.0f + character * 5.0f;
            const float nl = asymDrive (v1, drive, character * 0.6f);
            damped = kEff * lerp (v1, nl, character);
        }

        float hp2 = in - damped - v2;

        // --- 1st-order section, same corner -> 18 dB/oct total ---------------
        const float G1 = g / (1.0f + g);
        const float d  = (hp2 - z1) * G1;
        const float lp1 = d + z1;
        z1 = fixDenorm (lp1 + d);
        float y = hp2 - lp1;

        // Passive networks have insertion loss, and it grows with the amount of
        // peaking you dial in (you are trading termination for level).
        y *= 1.0f - 0.18f * impedance;

        return y;
    }

    /** Convenience: run a block in place. */
    void processBlock (float* data, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
            data[i] = process (data[i]);
    }

private:
    void updateDynamics() noexcept
    {
        // Slow core: 40 ms / 400 ms. Fast core: 1 ms / 25 ms.
        const float att = lerp (40.0f, 1.0f, dynamics);
        const float rel = lerp (400.0f, 25.0f, dynamics);
        env.setTimes (att, rel);
    }

    float fs = 44100.0f;

    // 2nd-order state
    float ic1 = 0.0f, ic2 = 0.0f, k = 1.4f;
    // 1st-order state
    float z1 = 0.0f;

    Smoother    gSmooth;
    EnvFollower env;
    SVF         clickFilter;

    float gTarget = 0.01f;
    float impedance = 0.35f, magnetism = 0.3f, character = 0.25f;
    float dynamics = 0.3f, artefacts = 0.2f, gain = 1.0f;
    float clickEnergy = 0.0f;
    float modOct = 0.0f;
    bool  pendingClick = false;

    int curType = -1, curStep = -1;
};

} // namespace wh
