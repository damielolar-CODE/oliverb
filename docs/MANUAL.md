# OLIVERB — User Manual

*Version 1.0.0*

OLIVERB is three pieces of 1960s dub hardware in one plug-in: a passive inductor-based
high-pass filter (the "Big Knob" of King Tubby's mixing desk), a two-track tape machine
patched into itself as an echo, and a spring reverb tank driven harder than its designer
intended. Use one section or all three — each has its own on/off switch.

**Formats:** VST3, AU, Standalone · **Platforms:** macOS (universal), Windows, Linux · **License:** MIT

---

## 1. Installation

### macOS

Download `OLIVERB-macOS.zip` from the
[Releases page](https://github.com/damielolar-CODE/oliverb/releases), unzip it, and run
**`OLIVERB-macOS.pkg`**. It installs the VST3 and AU for all users:

- `/Library/Audio/Plug-Ins/VST3/OLIVERB.vst3`
- `/Library/Audio/Plug-Ins/Components/OLIVERB.component`

The installer is unsigned, so the first launch may be blocked. Right-click the `.pkg` →
**Open** → **Open** anyway. If the plug-in itself is quarantined:

```
xattr -dr com.apple.quarantine /Library/Audio/Plug-Ins/VST3/OLIVERB.vst3
```

Prefer a manual install? The zip also contains the raw `OLIVERB.vst3` and
`OLIVERB.component` — drop them into `~/Library/Audio/Plug-Ins/VST3/` and
`~/Library/Audio/Plug-Ins/Components/`.

### Windows

Download `OLIVERB-Windows.zip`, unzip, and run **`OLIVERB-Windows-Setup.exe`**. It
installs to `C:\Program Files\Common Files\VST3\OLIVERB.vst3`. SmartScreen will warn
about an unrecognised app: click **More info → Run anyway**. Manual install: copy the
`OLIVERB.vst3` folder from the zip into `C:\Program Files\Common Files\VST3\`.

### Linux

Download `OLIVERB-Linux.zip` and copy `OLIVERB.vst3` to `~/.vst3/`.

After installing on any platform, rescan plug-ins in your DAW. OLIVERB appears under
**Deestech** in the Fx / Filter / Delay / Reverb categories.

---

## 2. First sound in sixty seconds

1. Insert OLIVERB on a drum loop or a full mix.
2. Open the preset menu and pick **Waterhouse Rockers**. That's the classic chain:
   filter into echo into spring.
3. Grab the big **Frequency** dial and sweep it up and back down. The corner peak,
   the switch clacks and the way the echo tail follows the dial — that's the plug-in.
4. Turn **Feedback** up past 80 % and tap **Send** to *Held*: the input stops feeding
   the machine but the loop keeps regenerating. That's a dub-out.
5. Tap Send back to *Dub* and carry on.

---

## 3. Signal flow

```
              ┌───────────  PRE (default)  ───────────┐
   input ──►  FILTER  ──►  ECHO  ──►  SPRING  ──►  OUTPUT
              └── or POST: ECHO ──► SPRING ──► FILTER ─┘
```

The **Pre / Post** switch is the most important routing decision in the plug-in:

- **Pre** — the filter feeds the echo, so every repeat inherits the dial position, and
  sweeping the dial drags the echo tail with it. This is the performance position.
- **Post** — the filter sits across the finished signal, echo tails included. Use it
  to carve the whole wet mix at once.

Everything runs at 2× oversampling internally; the extra latency is reported to your
host automatically, so tracks stay aligned.

---

## 4. The Filter — Big Dial

A passive, inductor-based high-pass, third order (18 dB/octave). No active parts — its
character comes from termination and core saturation, not resonance.

| Control | Range (default) | What it does |
|---|---|---|
| **Frequency** | steps 1–11 (1) | Eleven switch positions. See the table below. |
| **Type** | A / B (A) | Capacitor bank. A is the classic broad sweep; B sits lower and tighter. |
| **Impedance** | 0–100 % (35 %) | Termination. At 0 % the network is properly terminated and flat; open it up and the corner lifts by several dB. This peak is the sound of the hardware. |
| **Magnetism** | 0–100 % (30 %) | How hard the inductor cores saturate. Signal level pushes the corner upward and generates harmonics. |
| **Character** | 0–100 % (25 %) | Non-linearity in the damping path — adds harmonic content (even and odd) independent of the core. |
| **Dynamics** | 0–100 % (30 %) | How fast the core tracks the signal. Slow = a gentle breathing lift; fast = it behaves like an envelope filter. |
| **Artefacts** | 0–100 % (25 %) | Switch thump. At 0 % the corner glides between positions (the hardware can't do this). At 100 % it clacks like the real rotary. |
| **Gain** | ±18 dB (0 dB) | Internal trim after the filter. |
| **Pre / Post** | (Pre) | Filter before the echo, or across the finished signal. See §3. |

### Switch positions

| Step | Bank A | Bank B |
|---|---|---|
| 1 | 70 Hz | 50 Hz |
| 2 | 100 Hz | 80 Hz |
| 3 | 150 Hz | 120 Hz |
| 4 | 220 Hz | 170 Hz |
| 5 | 330 Hz | 240 Hz |
| 6 | 500 Hz | 350 Hz |
| 7 | 750 Hz | 520 Hz |
| 8 | 1.1 kHz | 800 Hz |
| 9 | 1.8 kHz | 1.3 kHz |
| 10 | 3.5 kHz | 2.4 kHz |
| 11 | 7.5 kHz | 5.0 kHz |

---

## 5. The Echo — Two Track

A studio tape recorder used as an echo: record head → tape → replay head, output patched
back to the input. Each pass through the loop loses a little top end and gains a little
saturation, so long tails get darker and thicker rather than louder.

| Control | Range (default) | What it does |
|---|---|---|
| **Time** | 20–2000 ms (375 ms) | Tape delay. Changing it drags the transport, so pitch bends on the way — as tape does. |
| **Sync / Free** | (Free) | Lock Time to the host tempo using the Division selector. |
| **Division** | 1/16 – 1/1 (1/4) | Note length when synced. Includes triplet (T) and dotted (.) values. |
| **Feedback** | 0–100 % (34 %) | Loop regeneration. Unity gain sits at 80 % — above that the machine self-oscillates on purpose, limiting into the record amp instead of exploding. |
| **Send** | Dub / Held (Dub) | *Dub* feeds the input into the machine. *Held* closes the door: the input stops, but whatever is on the loop keeps circulating. Throw a snare in, then hold it. |
| **Input** | 0–100 % (57 %) | Level into the record amp. Push it to saturate the tape harder. |
| **Output** | 0–100 % (60 %) | Level out of the machine. |
| **Hiss** | 0–100 % (18 %) | Tape noise, recorded *to* the tape — it recirculates and builds with feedback instead of sitting on top. |
| **Mix** | 0–100 % (32 %) | Dry/wet balance for the echo. |
| **Wear** | 0–100 % (45 %) | Machine condition: one knob scaling flutter, high-frequency loss, head bump and record-amp drive from freshly-aligned to tired. |

---

## 6. Mod — LFO / Envelope

Both modulation sources push the **Frequency dial**, in octaves relative to the switch
position. Everything the filter does — corner peak, core saturation, character — rides
along with the sweep. Modulation is shared by both channels, so the stereo image holds.

### LFO

| Control | Range (default) | What it does |
|---|---|---|
| **LFO On** | (Off) | Enables the LFO. |
| **Shape** | Sine / Triangle / Saw Down / Square / S+H (Sine) | Square and sample-and-hold are lightly smoothed: the corner snaps, the zipper doesn't. |
| **Rate** | 0.02–20 Hz (0.8 Hz) | Free-running speed. |
| **Sync / Free** | (Free) | Lock the rate to the host tempo; synced sweeps re-align to the bar. |
| **Division** | 1/16 – 1/1 (1/2) | Note length when synced. |
| **Depth** | ±3 oct (0) | How far the dial travels. Negative inverts the sweep. |

### Envelope follower

Follows the **input** signal (pre-filter), so it responds to what you play, not to what
the filter is already doing.

| Control | Range (default) | What it does |
|---|---|---|
| **Env Depth** | ±3 oct (0) | Bipolar: positive opens the filter on hits (auto-wah), negative ducks it out of the way. |
| **Sens** | 0–100 % (50 %) | How hard the follower listens. |
| **Speed** | 0–100 % (50 %) | How fast it moves. |

---

## 7. The Spring — Tank

A two-tank spring reverb. Springs are dispersive — highs travel through the coil faster
than lows — so a transient smears into the characteristic descending *boing* that room
reverbs never produce.

| Control | Range (default) | What it does |
|---|---|---|
| **Spring** | 0–100 % (22 %) | How much of the tank is in the output. |
| **Tension** | 0–100 % (55 %) | Decay time of the tank. |
| **Drive** | 0–100 % (25 %) | Level into the send transducer. This is where the crash lives — hit it hard and transients splash. |

---

## 8. Global

| Control | Range (default) | What it does |
|---|---|---|
| **Output** | −24 to +12 dB (0 dB) | Final level trim. |
| **Bypass** | (Off) | True bypass of the whole plug-in. |

Each of the three sections also has its own **On** switch, so OLIVERB can serve as just
a filter, just an echo, or just a spring.

---

## 9. Factory presets

| Preset | What it shows |
|---|---|
| **Init** | Everything at defaults. |
| **Waterhouse Rockers** | The classic chain at working settings — the place to start. |
| **Snare Throw** | High feedback, hot input: hit it with one snare and ride the tail. |
| **Big Dial Sweep** | Open termination, heavy magnetism, audible switch clacks — for performing the dial. |
| **Tape Wash** | Long, worn, hissy repeats that melt into the spring. |
| **Held Echo (Dub Out)** | Send is *Held* and feedback at unity: an infinite loop, filter across the output. Open the Send to let new signal in. |
| **Spring Crash** | The tank up front, driven hard. |
| **Roots Bass Tighten** | Filter only, step 1, a little core saturation — a bass-tightening tool, no wet signal at all. |
| **Auto Wah Skank** | Envelope follower opening the dial on hits. Guitars and clavs. |
| **Tidal Sweep** | Tempo-synced triangle LFO sweeping two octaves over a whole bar. |
| **Siren** | Everything at maximum. You were warned. |

---

## 10. Techniques

- **The dub throw.** Feedback high, Mix high, Send on *Dub*. Un-mute (or hot-cue) one
  hit — a snare, a vocal word — then flip Send to *Held*. The hit circulates and decays
  on its own while the dry signal carries on. Flip back to *Dub* for the next throw.
- **Ride the dial.** With the filter **Pre**, sweep Frequency during the echo tail: every
  repeat re-inherits the new corner and the whole tail bends. Add **Artefacts** for the
  hardware clack on each step.
- **Pitch-bend echoes.** Automate **Time**. The transport drags, so repeats bend like
  varispeed tape. Small moves = subtle warble; big jumps = dive-bombs.
- **Self-oscillation instrument.** Feedback past 80 %, no input: the machine sings on
  its own. Tune it with Time, filter it with the dial, crash it into the spring.
- **Auto-wah.** Env Depth positive, filter step 4–6, Impedance up. Sens sets how hard
  it listens, Speed how fast the corner chases the playing.
- **Reverse duck.** Env Depth *negative*: the filter closes on hits and blooms back
  open in the gaps — an ungate for pads and textures.

---

## 11. Troubleshooting

| Symptom | Fix |
|---|---|
| Plug-in doesn't appear in the DAW | Rescan plug-ins. Confirm the `.vst3` is in the system VST3 folder (§1). On Apple-silicon Macs check the DAW isn't running in Rosetta with an arm64-only scan cache. |
| macOS blocks the installer or plug-in | Right-click → Open on the `.pkg`, or clear quarantine with the `xattr` command in §1. |
| Windows SmartScreen warning | More info → Run anyway. The installer is unsigned, not unsafe. |
| Sound is late / flamming when bypassed elsewhere | OLIVERB reports its 2× oversampling latency to the host; enable your DAW's plug-in delay compensation. |
| Echo tail never dies | Feedback is at or above 80 % (unity). That's a feature — pull it down or flip Send to *Dub* with no input. |
| Output slammed after big dial moves | Open Impedance + hot echo feedback genuinely adds level. Use the filter **Gain** or global **Output** trim. The output is hard-limited at the end of the chain, so it cannot run away. |

---

## 12. Specifications

- **Formats:** VST3, Audio Unit (macOS), Standalone application
- **Platforms:** macOS 10.13+ (universal: Apple silicon + Intel), Windows 10+ (64-bit), Linux
- **Processing:** 32-bit float, 2× oversampled (half-band polyphase IIR), latency-compensated
- **Filter:** third-order (18 dB/oct) passive constant-k model, TPT state-variable core, stable under audio-rate modulation
- **Echo:** 20–2000 ms, wow & flutter, per-pass HF loss, in-loop record-amp saturation
- **Spring:** two dispersive tanks (33.7 ms / 41.9 ms coil transit), twelve allpass sections each
- **Source & license:** MIT, at [github.com/damielolar-CODE/oliverb](https://github.com/damielolar-CODE/oliverb)

*King Tubby's name and the names of any commercial products are used only to describe
the hardware being modelled; no affiliation or endorsement is claimed or implied.*
