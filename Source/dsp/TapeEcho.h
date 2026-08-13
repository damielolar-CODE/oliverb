// =============================================================================
//  OLIVERB — TapeEcho.h
//
//  A 2-track studio recorder pressed into service as an echo unit: record
//  head, a fixed gap to the replay head, and a hand-patched feedback loop
//  from output back to input. That is how the dub echoes were actually made —
//  not on a purpose-built echo box, but on the machine that was already there.
//
//  What that implies, and what is modelled here:
//
//   * Delay time = tape speed. You cannot step it; you change it by dragging
//     the transport, so every time change bends pitch on the way. TIME is
//     therefore slewed, not switched.
//
//   * Wow (slow, capstan/reel eccentricity, ~0.5 Hz) and flutter (fast,
//     scrape and guide chatter, ~7 Hz plus noise) ride on top permanently.
//
//   * Replay head response: a low-frequency head bump around 80-100 Hz from
//     the finite head gap, plus real HF loss on every pass. Repeats therefore
//     get darker and fatter each time round, which is most of the sound.
//
//   * The record amplifier is the first thing to distort when the feedback
//     loop is pushed. Feedback over unity self-oscillates and then limits
//     into that saturation rather than blowing up.
//
//   * Tape hiss is recorded to tape, so it recirculates and builds with the
//     feedback. It is injected before the delay line, not added at the output.
//
//   * The SEND ("dub") switch cuts the feed *into* the machine while leaving
//     the loop running — the classic move for throwing a snare into an echo
//     and then closing the door behind it.
// =============================================================================
#pragma once

#include "Utils.h"

namespace wh
{

class TapeEcho
{
public:
    static constexpr float kMaxDelaySeconds = 4.0f;
    static constexpr float kMinTimeMs = 20.0f;
    static constexpr float kMaxTimeMs = 2000.0f;

    void prepare (double sampleRate)
    {
        fs = static_cast<float> (sampleRate);

        for (int c = 0; c < 2; ++c)
        {
            auto& ch = chan[c];
            ch.line.prepare (sampleRate, kMaxDelaySeconds);
            ch.hfLoss.prepare (sampleRate);
            ch.lfCut.prepare (sampleRate);
            ch.bump.prepare (sampleRate);
            ch.bump.set (95.0f, 0.7f);
            ch.hissTone.prepare (sampleRate);
            ch.hissTone.set (3200.0f, 0.5f);
            ch.dc.prepare (sampleRate);
            ch.noise.reset (0x9E3779B9u + static_cast<uint32_t> (c) * 2654435761u);
            ch.fb = 0.0f;
        }

        timeSmooth.prepare (sampleRate, 140.0f);   // capstan inertia
        timeSmooth.snap (msToSamples (400.0f));

        wowPhase = 0.0f;
        flutPhase = 0.0f;
        reset();
        updateTone();
    }

    void reset()
    {
        for (auto& ch : chan)
        {
            ch.line.reset();
            ch.hfLoss.reset();
            ch.lfCut.reset();
            ch.bump.reset();
            ch.hissTone.reset();
            ch.dc.reset();
            ch.fb = 0.0f;
        }
    }

    // ---- Parameters ---------------------------------------------------------
    void setTimeMs (float ms) noexcept
    {
        timeSmooth.setTarget (msToSamples (clampf (ms, kMinTimeMs, kMaxTimeMs)));
    }

    /** Jump straight to a new time with no pitch bend (used on load / preset change). */
    void snapTimeMs (float ms) noexcept
    {
        timeSmooth.snap (msToSamples (clampf (ms, kMinTimeMs, kMaxTimeMs)));
    }

    void setFeedback (float v) noexcept { feedback = clampf (v, 0.0f, 1.25f); }
    void setInputLevel (float v) noexcept { inLevel = clampf (v, 0.0f, 2.0f); }
    void setOutputLevel (float v) noexcept { outLevel = clampf (v, 0.0f, 2.0f); }
    void setHiss (float v) noexcept { hiss = clampf (v, 0.0f, 1.0f); }
    void setMix (float v) noexcept { mix = clampf (v, 0.0f, 1.0f); }
    void setSend (bool on) noexcept { sendOpen = on; }
    void setAge (float v) noexcept { age = clampf (v, 0.0f, 1.0f); updateTone(); }

    // ---- Audio --------------------------------------------------------------
    void process (float& left, float& right) noexcept
    {
        const float baseDelay = timeSmooth.next();

        // Transport instability. Wow is slow and roughly sinusoidal; flutter is
        // faster and noisy. Depth scales with AGE (a tired machine drifts more).
        wowPhase  += kTwoPi * 0.47f / fs;   if (wowPhase  > kTwoPi) wowPhase  -= kTwoPi;
        flutPhase += kTwoPi * 7.3f  / fs;   if (flutPhase > kTwoPi) flutPhase -= kTwoPi;

        const float depth = 0.0012f + 0.0060f * age;
        const float wow   = std::sin (wowPhase);
        const float flut  = std::sin (flutPhase) * 0.45f + wobbleNoise.next() * 0.12f;

        const float dryL = left, dryR = right;

        for (int c = 0; c < 2; ++c)
        {
            auto& ch = chan[c];
            const float phaseOff = (c == 0 ? 0.0f : 0.35f);     // heads are not identical
            const float modA = std::sin (wowPhase + phaseOff);
            const float mod  = baseDelay * (1.0f + depth * (0.72f * modA + 0.28f * flut + 0.0f * wow));

            // --- replay head ------------------------------------------------
            float tapeOut = ch.line.read (clampf (mod, 4.0f, static_cast<float> (ch.line.length() - 8)));

            tapeOut = ch.lfCut.highpass (tapeOut);                 // no DC/rumble on tape
            tapeOut += ch.bump.bandpass (tapeOut) * (0.35f + 0.25f * age);   // head bump
            tapeOut = ch.hfLoss.lowpass (tapeOut);                 // gap loss
            tapeOut = ch.dc.process (tapeOut);

            // --- record amp -------------------------------------------------
            const float dry = (c == 0 ? dryL : dryR);
            float rec = (sendOpen ? dry * inLevel : 0.0f) + tapeOut * feedback;

            if (hiss > 0.0f)
            {
                const float n = ch.hissTone.lowpass (ch.noise.next());
                rec += n * hiss * hiss * 0.02f;
            }

            // Saturation lives in the record amp: this is what stops runaway
            // feedback from exploding and what makes long repeats "cook".
            rec = asymDrive (rec, 1.0f + age * 0.8f, 0.18f);

            ch.line.write (fixDenorm (rec));

            const float wet = tapeOut * outLevel;
            float& out = (c == 0 ? left : right);
            out = lerp (dry, wet, mix);
        }
    }

    /** Wet-only output — used when the echo feeds the spring tank in series. */
    float currentTimeSamples() const noexcept { return timeSmooth.value(); }

private:
    static constexpr float kTwoPi = 2.0f * kPi;

    float msToSamples (float ms) const noexcept { return ms * 0.001f * fs; }

    void updateTone()
    {
        // A well-aligned machine gets to ~10 kHz at 15ips; a worn one much less.
        const float hf = lerp (9000.0f, 2600.0f, age);
        for (auto& ch : chan)
        {
            ch.hfLoss.setCutoff (hf);
            ch.lfCut.setCutoff (45.0f);
        }
    }

    struct Channel
    {
        DelayLine line;
        OnePole   hfLoss, lfCut;
        SVF       bump, hissTone;
        DCBlocker dc;
        Noise     noise;
        float     fb = 0.0f;
    };

    Channel chan[2];
    Smoother timeSmooth;
    Noise wobbleNoise { 0xC0FFEEu };

    float fs = 44100.0f;
    float feedback = 0.35f, inLevel = 0.8f, outLevel = 0.8f;
    float hiss = 0.15f, mix = 0.35f, age = 0.45f;
    float wowPhase = 0.0f, flutPhase = 0.0f;
    bool  sendOpen = true;
};

} // namespace wh
