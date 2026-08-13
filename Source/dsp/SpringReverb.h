// =============================================================================
//  OLIVERB — SpringReverb.h
//
//  A two-tank spring reverb, of the sort bolted into the back of a 1960s
//  console and then "modified" (read: run hotter than the designer intended).
//
//  Springs do not behave like rooms. A room is a dense cloud of reflections;
//  a spring is a transmission line that is *dispersive* — high frequencies
//  travel through the coil faster than low ones, so a transient smears into
//  the descending "boing" chirp that everyone recognises. Reverb algorithms
//  built from plain comb/allpass room networks never get this, because
//  dispersion is the whole point.
//
//  So the structure here is:
//
//    input -> band limit (a spring only transmits ~80 Hz - 4.5 kHz)
//          -> tank loop:  delay (coil transit time)
//                         -> long chain of allpass sections (dispersion)
//                         -> damping (wire and transducer losses)
//                         -> back into the delay
//
//  Two tanks with slightly different transit times and dispersion run in
//  parallel and are panned apart, which is what a real two-spring tank does
//  and why spring reverb sounds wide from a mono source.
// =============================================================================
#pragma once

#include "Utils.h"

namespace wh
{

class SpringReverb
{
public:
    void prepare (double sampleRate)
    {
        fs = static_cast<float> (sampleRate);

        // Allpass lengths: mutually prime, so the chain does not develop
        // a periodic ring of its own.
        static const int primes[kNumAP] =
            { 23, 41, 59, 79, 97, 113, 137, 157, 173, 191, 211, 229 };

        for (int t = 0; t < kNumTanks; ++t)
        {
            auto& tank = tanks[t];

            // Transit time of the coil, ~30-45 ms. The two tanks are detuned.
            const float ms = (t == 0 ? 33.7f : 41.9f);
            tank.delaySamples = ms * 0.001f * fs;
            tank.line.prepare (sampleRate, 0.2f);

            for (int i = 0; i < kNumAP; ++i)
            {
                const int len = static_cast<int> (primes[i] * (fs / 44100.0f)) + 1 + t * 3;
                tank.ap[i].prepare (len, 0.62f);
            }

            tank.lowCut.prepare (sampleRate);
            tank.lowCut.setCutoff (95.0f);
            tank.damp.prepare (sampleRate);
            tank.damp.setCutoff (4200.0f);
            tank.body.prepare (sampleRate);
            tank.body.set (t == 0 ? 1650.0f : 2100.0f, 0.9f);
            tank.dc.prepare (sampleRate);
            tank.modPhase = t * 1.7f;
        }

        inputBand.prepare (sampleRate);
        inputBand.setCutoff (4800.0f);
        inputCut.prepare (sampleRate);
        inputCut.setCutoff (110.0f);

        reset();
    }

    void reset()
    {
        for (auto& tank : tanks)
        {
            tank.line.reset();
            for (auto& a : tank.ap) a.reset();
            tank.lowCut.reset();
            tank.damp.reset();
            tank.body.reset();
            tank.dc.reset();
        }
        inputBand.reset();
        inputCut.reset();
    }

    /** 0 = dry, 1 = drowning in it. */
    void setAmount (float v) noexcept { amount = clampf (v, 0.0f, 1.0f); }

    /** Loop decay: how lossy the wire and transducers are. */
    void setDecay (float v) noexcept { decay = clampf (v, 0.0f, 1.0f); }

    /** Drive into the tank — over-driving the send transducer is a dub staple. */
    void setDrive (float v) noexcept { drive = clampf (v, 0.0f, 1.0f); }

    void process (float& left, float& right) noexcept
    {
        const float dryL = left, dryR = right;
        if (amount <= 0.0001f) return;

        float in = 0.5f * (dryL + dryR);
        in = inputCut.highpass (in);
        in = inputBand.lowpass (in);

        if (drive > 0.001f)
            in = asymDrive (in, 1.0f + drive * 6.0f, 0.25f) * (1.0f - 0.35f * drive);

        // Loop gain must stay below unity or the tank turns into an oscillator.
        // Every stage inside the loop is unity-or-less on purpose (the allpass
        // chain is magnitude-flat, the body resonance is blended in rather than
        // added), so this single number sets the decay time.
        const float fb = lerp (0.55f, 0.84f, decay);

        float wet[kNumTanks] = { 0.0f, 0.0f };

        for (int t = 0; t < kNumTanks; ++t)
        {
            auto& tank = tanks[t];

            // Very small length modulation: a real tank is never perfectly still.
            tank.modPhase += kPi * 2.0f * (t == 0 ? 0.31f : 0.43f) / fs;
            if (tank.modPhase > kPi * 2.0f) tank.modPhase -= kPi * 2.0f;
            const float d = tank.delaySamples * (1.0f + 0.0009f * std::sin (tank.modPhase));

            float v = tank.line.read (d);

            // Dispersion: this chain is what turns a click into a chirp.
            for (auto& a : tank.ap)
                v = a.process (v);

            v = tank.lowCut.highpass (v);
            v = tank.damp.lowpass (v);
            v = lerp (v, tank.body.bandpass (v), 0.35f);   // metallic coil resonance
            v = tank.dc.process (v);
            v = softClip (v);                              // transducer limiting

            tank.line.write (fixDenorm (in * 0.6f + v * fb));
            wet[t] = v;
        }

        // Tanks are physically separated, so they pan apart.
        const float wl = wet[0] * 0.78f + wet[1] * 0.32f;
        const float wr = wet[1] * 0.78f + wet[0] * 0.32f;

        const float w = amount * amount;             // taper: usable at low settings
        left  = dryL + wl * w * 1.35f;
        right = dryR + wr * w * 1.35f;
    }

private:
    static constexpr int kNumTanks = 2;
    static constexpr int kNumAP = 12;

    struct Tank
    {
        DelayLine line;
        Allpass   ap[kNumAP];
        OnePole   lowCut, damp;
        SVF       body;
        DCBlocker dc;
        float     delaySamples = 1500.0f;
        float     modPhase = 0.0f;
    };

    Tank tanks[kNumTanks];
    OnePole inputBand, inputCut;

    float fs = 44100.0f;
    float amount = 0.25f, decay = 0.5f, drive = 0.2f;
};

} // namespace wh
