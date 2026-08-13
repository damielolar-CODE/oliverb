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

#include <cmath>
#include <cstdio>
#include <string>

namespace
{
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
    const double w = 2.0 * M_PI * hz / kSR;

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
    std::printf ("\nEcho: runaway feedback is contained\n");
    {
        wh::TapeEcho e;
        e.prepare (kSR);
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
