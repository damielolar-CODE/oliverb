# Design notes

Written so that future-you (or anyone else) can change the sound without reverse
engineering the code first. Each section says what the hardware does, how it's modelled,
and which line to touch.

---

## 1. The filter — `Source/dsp/PassiveHighPass.h`

### The hardware

A passive constant-k / T-network high-pass: series capacitors, shunt inductors, no active
parts anywhere. Three reactive elements, so third order, so **18 dB/octave**. A rotary
switch swaps capacitor values to move the corner in eleven discrete steps.

Three things about it matter musically, and none of them are in a textbook filter:

1. **The peak at the corner isn't resonance, it's termination.** The network is designed
   into 600 Ω. Under-terminate it and the corner lifts by several dB. That's `Impedance`.
2. **The inductors saturate.** Real core, real hysteresis. Push level in and the
   inductance falls, so the corner climbs *with the signal*, and harmonics appear. That's
   `Magnetism`, with `Dynamics` setting how fast the core follows.
3. **The switch clacks.** Caps get shorted and reconnected under signal. That's `Artefacts`.

### The model

A resonant 2nd-order TPT state-variable section in series with a 1st-order TPT section at
the same corner: same slope, same corner, cheap and unconditionally stable under audio-rate
cutoff modulation (which a direct-form biquad is not — worth remembering if you're tempted
to swap it out).

Per sample:

```
env  = envelope of the bandpass state (≈ inductor current)
g    = g_base * (1 + magnetism * f(env))     // core saturation lifts the corner
kEff = k * (1 + magnetism * env * 0.8)       // a saturating core is a lossy core
```

`g` is scaled directly rather than recomputing `tan()` per sample. It's an approximation
that gets slightly optimistic at very high corners, and it costs one division per sample
instead of a transcendental.

`Character` blends the damping path between linear and asymmetrically driven
(`asymDrive` in `Utils.h`). Asymmetric on purpose: symmetric clipping is odd-harmonic only
and sounds like a fuzz pedal, while the bias term brings in even harmonics.

**Tweak points**

| Want | Change |
|---|---|
| Different switch frequencies | `stepFrequency()` — the two tables at the top |
| More/less corner peak | the `Q` range in `setImpedance()` (currently 0.62 → 3.2) |
| More dramatic core saturation | the `2.6f` / `1.8f` constants in the `lift` line |
| Louder or duller switch clack | `clickFilter.set (180.0f, 0.9f)` in `prepare()` |
| Steeper filter | add another `OnePole` stage → 24 dB/oct (and update `dsp_test`) |

---

## 2. The echo — `Source/dsp/TapeEcho.h`

A recorder, not an echo box: record head → gap → replay head → patch the output back to
the input.

The loop, in order: read the tape (modulated by wow and flutter) → replay-head response
(low cut, head bump, HF loss) → DC block → record amp (input + feedback + hiss, then
saturation) → write.

Design decisions worth knowing about:

- **Time is slewed, not switched** (`Smoother` at 140 ms). You change tape delay by
  changing speed, so pitch bends on the way. Use `snapTimeMs()` when you genuinely want
  an instant jump (preset loads do this).
- **Saturation lives in the record amp, inside the loop.** That's what makes feedback
  above unity settle into a limit cycle instead of diverging, and it's why long repeats
  get thicker rather than louder. The test harness asserts this at feedback 1.25.
- **Hiss is injected before the delay line**, so it recirculates and builds with feedback,
  as tape noise does. Adding it at the output would be wrong and would sound wrong.
- **HF loss is per pass**, so each repeat is darker than the last. Almost all of the
  "tape" character is here and in the head bump.

`Wear` scales flutter depth, HF loss, head bump and record-amp drive together — one knob
from a freshly aligned machine to a tired one.

---

## 3. The spring — `Source/dsp/SpringReverb.h`

Springs are *dispersive*: highs travel through the coil faster than lows, so a transient
smears into the descending chirp. Room-reverb networks never produce this, which is why
they never sound like springs.

Per tank: a delay line (coil transit time, 33.7 ms and 41.9 ms for the two tanks) with a
chain of twelve mutually-prime allpass sections inside the loop. The allpasses are
magnitude-flat and phase-nonlinear — that chain *is* the dispersion. Then a low cut, a
damping low-pass, a blended body resonance, and soft clipping for the transducer.

**Loop gain must stay under unity.** The single `fb` value (0.55 → 0.84) sets decay
because every other stage in the loop is unity-or-less by construction. The body resonance
is *blended* rather than added for exactly this reason — adding it put loop gain over 1 and
turned the tank into an oscillator. `dsp_test` catches that: it checks that tail energy
after 7 seconds is well below energy at 1 second.

**Tweak points**: `ms` in `prepare()` for tank size, the `primes[]` table and `0.62f`
allpass coefficient for how chirpy it is, `body.set()` for the metallic ring.

---

## 3½. Modulation — `Source/dsp/Modulation.h`

The LFO and the envelope follower both write to `PassiveHighPass::setModOctaves()`,
which scales the integrator gain by `2^octaves` per sample. Scaling `g` directly (rather
than recomputing `tan()`) is exact for small `g` and slightly flat at the top of the
range — the same trade the core-saturation model already makes, and it keeps a
transcendental out of the per-sample path. The TPT topology is what makes audio-rate
corner modulation safe at all; a direct-form biquad would blow up here.

Two decisions worth keeping:

- **Modulation is shared by both channels.** Independent L/R modulation makes the stereo
  image wander; the hardware's filter was mono anyway.
- **The envelope listens to the input, pre-filter.** Following the filter's own output
  would close a feedback loop around the corner position — briefly entertaining, mostly
  unstable.

Square and S&H run through a 3 ms smoother: the corner still snaps, the zipper doesn't.
`dsp_test` sweeps all five shapes at ±3 octaves against the filter's worst-case settings
and asserts the output stays finite and bounded.

## 4. Plumbing — `Source/PluginProcessor.cpp`

- Everything runs at **2× oversampling** (`filterHalfBandPolyphaseIIR`), because both the
  core model and the record amp generate harmonics that would otherwise fold back down.
  Latency is reported to the host automatically.
- Mono and stereo hosts share one code path: the input is widened to two channels before
  the oversampler, then narrowed on the way out.
- Output is clamped to ±4 and NaN-checked. A wide-open termination feeding a hot echo loop
  can genuinely hand back more than it was given, and a NaN in a feedback loop is permanent.
- Presets reset every parameter to its default first, then apply their own deltas —
  otherwise preset B silently inherits whatever preset A left behind.

## 5. Adding a parameter

1. Add the ID in `Parameters.h` and one line in `createParameterLayout()`.
2. Cache a pointer in the processor's constructor, push it to the DSP in `pullParameters()`.
3. Add a `KnobBox` in the editor's constructor and give it a slot in `layoutContent()`.

Nothing else needs touching — state saving and host automation follow the layout.
