// =============================================================================
//  Waterhouse — Modulation.h
//
//  The LFO that drives the big dial. Kept deliberately plain: five shapes,
//  free-running or host-synced, output in the range [-1, 1].
//
//  The envelope side of modulation reuses wh::EnvFollower from Utils.h —
//  the same class the filter's core model uses — so there is exactly one
//  envelope implementation in the project.
// =============================================================================
#pragma once

#include "Utils.h"

namespace wh
{

class LFO
{
public:
    enum Shape { Sine = 0, Triangle, SawDown, Square, SampleHold, NumShapes };

    void prepare (double sampleRate) noexcept
    {
        fs = static_cast<float> (sampleRate);
        phase = 0.0f;
        held = 0.0f;
        lastPhase = 1.0f;   // force a fresh S&H value on the first cycle
        smooth.prepare (sampleRate, 3.0f);
        smooth.snap (0.0f);
    }

    void setShape (int s) noexcept { shape = std::max (0, std::min ((int) NumShapes - 1, s)); }
    void setRateHz (float hz) noexcept { rate = clampf (hz, 0.01f, 30.0f); }

    /** Re-align the phase to the host timeline so synced sweeps land on the bar. */
    void resync (double ppqPosition, float cycleQuarterNotes) noexcept
    {
        if (cycleQuarterNotes > 0.0f)
        {
            const double cycles = ppqPosition / (double) cycleQuarterNotes;
            phase = static_cast<float> (cycles - std::floor (cycles));
        }
    }

    /** Advance one sample; returns [-1, 1]. */
    float next() noexcept
    {
        lastPhase = phase;
        phase += rate / fs;
        if (phase >= 1.0f) phase -= 1.0f;

        float v = 0.0f;
        switch (shape)
        {
            case Sine:       v = std::sin (phase * 2.0f * kPi); break;
            case Triangle:   v = 4.0f * std::fabs (phase - 0.5f) - 1.0f; break;
            case SawDown:    v = 1.0f - 2.0f * phase; break;
            case Square:     v = phase < 0.5f ? 1.0f : -1.0f; break;
            case SampleHold:
                if (phase < lastPhase)          // wrapped: draw a new value
                    held = rng.next();
                v = held;
                break;
            default: break;
        }

        // A touch of smoothing keeps square and S&H from clicking when the
        // depth is high — the corner still snaps, the zipper doesn't.
        smooth.setTarget (v);
        return smooth.next();
    }

private:
    float fs = 44100.0f, rate = 1.0f, phase = 0.0f, lastPhase = 1.0f, held = 0.0f;
    int shape = Sine;
    Noise rng { 0xB16D1A1u };
    Smoother smooth;
};

} // namespace wh
