// =============================================================================
//  OLIVERB — ValveStage.h
//
//  A single-ended triode preamp stage with an output transformer: the valve
//  line amplifier that sat in front of (and behind) everything in a 1960s
//  studio. No reverb, no delay — just the box that made the console warm.
//
//  What is modelled, and why it sounds like a valve rather than a clipper:
//
//   * The transfer curve is asymmetric (see wh::triode). Grid conduction
//     flattens the positive swing quickly; cut-off softens the negative swing
//     slowly. Even-order harmonics fall out of that asymmetry — the 2nd
//     harmonic an octave above the note is the "warmth", the 3rd is the "bite".
//
//   * BIAS sets the operating point. Cold bias is nearly symmetric and stays
//     clean until pushed hard; hot bias is asymmetric from the first volt and
//     thick at any level.
//
//   * SAG is the power supply and cathode network giving way under load: a
//     slow envelope of the signal pulls the gain down and the bias colder.
//     That is the compression and "bloom" of a driven valve amp, and it is
//     what makes the stage feel like it pushes back.
//
//   * TONE is the output transformer: iron adds a low bump around 90 Hz and
//     loses the very top. Turned up, it also lets the transformer core clip
//     a touch, which is a different, rounder flavour than the valve itself.
//
//   * Small-signal gain is unity-normalised and level is compensated as the
//     drive goes up, so DRIVE changes the *shape* of the sound far more than
//     its loudness — you can perform it without the mix jumping.
//
//  Runs inside the plugin's 2x oversampled block; the harmonics it makes need
//  the headroom.
// =============================================================================
#pragma once

#include "Utils.h"

namespace wh
{

class ValveStage
{
public:
    void prepare (double sampleRate)
    {
        fs = static_cast<float> (sampleRate);
        for (auto& ch : chan)
        {
            ch.sag.prepare (sampleRate);
            ch.sag.setTimes (12.0f, 260.0f);
            ch.dc.prepare (sampleRate);
            ch.hf.prepare (sampleRate);
            ch.iron.prepare (sampleRate);
            ch.iron.set (92.0f, 0.55f);
        }
        driveSmooth.prepare (sampleRate, 25.0f);
        driveSmooth.snap (drive);
        reset();
        updateTone();
    }

    void reset()
    {
        for (auto& ch : chan)
        {
            ch.dc.reset();
            ch.hf.reset();
            ch.iron.reset();
        }
        glow = 0.0f;
    }

    // ---- Parameters ---------------------------------------------------------
    void setDrive (float v) noexcept { drive = clampf (v, 0.0f, 1.0f); driveSmooth.setTarget (drive); }
    void setBias  (float v) noexcept { bias  = clampf (v, 0.0f, 1.0f); }
    void setSag   (float v) noexcept { sag   = clampf (v, 0.0f, 1.0f); }
    void setTone  (float v) noexcept { tone  = clampf (v, 0.0f, 1.0f); updateTone(); }
    void setMix   (float v) noexcept { mix   = clampf (v, 0.0f, 1.0f); }

    /** How hard the valve is working right now, 0..1 — for the front-panel lamp. */
    float glowLevel() const noexcept { return glow; }

    // ---- Audio --------------------------------------------------------------
    void process (float& left, float& right) noexcept
    {
        const float d    = driveSmooth.next();
        const float pre  = 1.0f + d * d * 60.0f;                        // up to ~ +36 dB into the grid
        const float post = (1.0f + 0.0056f * (pre - 1.0f)) / pre;       // ... and back out: fitted so a
                                                                        // nominal sine keeps its RMS
                                                                        // within 1 dB across the range

        // The curve is run at a fraction of the nominal level so that zero
        // drive is a valve line amp doing its job (about 1% 2nd harmonic at
        // nominal level) rather than a valve already being pushed. kOp is the
        // operating level; the stage is unity-normalised so it costs nothing.
        constexpr float kOp = 0.09f;

        float hardest = 0.0f;

        for (int c = 0; c < 2; ++c)
        {
            auto& ch = chan[c];
            float& io = (c == 0 ? left : right);
            const float dry = io;

            // Supply sag: a slow envelope of the signal at the grid pulls the
            // gain down and the bias colder. This is the compression.
            const float env    = ch.sag.process (dry * pre * 0.12f);
            const float envC   = clampf (env, 0.0f, 1.5f);
            const float gSag   = 1.0f / (1.0f + sag * 1.6f * envC);
            const float biasEf = clampf (bias - sag * 0.35f * envC, 0.0f, 1.0f);

            // The valve
            float v = triode (dry * pre * gSag * kOp, biasEf) * (post / kOp);

            // Output transformer: iron bump below, gap loss above, and the
            // core rounding off anything that is still too big for it.
            v += ch.iron.bandpass (v) * (0.30f * tone);
            v  = ch.hf.lowpass (v);
            if (tone > 0.0f)
                v = lerp (v, softClip (v * 1.35f) / 1.35f, tone * 0.5f);

            v = ch.dc.process (v);

            io = lerp (dry, v, mix);

            hardest = std::max (hardest, envC * d);
        }

        // Lamp ballistics: fast up, slow down.
        glow += (hardest > glow ? 0.02f : 0.0008f) * (hardest - glow);
    }

private:
    void updateTone()
    {
        const float hf = lerp (19000.0f, 6500.0f, tone);
        for (auto& ch : chan)
            ch.hf.setCutoff (std::min (hf, fs * 0.45f));
    }

    struct Channel
    {
        EnvFollower sag;
        DCBlocker   dc;
        OnePole     hf;
        SVF         iron;
    };

    Channel  chan[2];
    Smoother driveSmooth;

    float fs = 44100.0f;
    float drive = 0.35f, bias = 0.5f, sag = 0.3f, tone = 0.4f, mix = 1.0f;
    float glow = 0.0f;
};

} // namespace wh
