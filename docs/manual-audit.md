# Official manual and API audit

Audit date: 2026-08-08
Audited implementation: `17d47c03e5bc0808fb54cba7abde4d9b79a947f2`
Remediation: implemented in the current candidate lineage; exact identity is
recorded in the acceptance evidence
Scope: the current Expert Sleepers disting NT manual and C++ API, the Mutable
Instruments Marbles printed and web manuals, the final first-party Marbles
source, and this repository's implementation, documentation, and tests.

## Verdict

The port now has the original T and X/Y generators, the six factory scales,
all seven outputs, the main musical parameters, working NT routing, and a live
custom UI. The owner's observation that it now produces rhythms and audible
scale changes is consistent with the implementation.

The audit found three required defects in the baseline: the original DSP's
fixed-call cadence had not been adapted to the NT's configurable engine block,
the custom UI could not keep soft takeover synchronized when a pot target
changed, and the held-pot T Range mapping made `4x` practically unreachable.
It also found missing output-bus modes, non-original loop lengths, opaque Y
divider labels, an unregistered reseed departure, and parameter-effect evidence
gaps. The current candidate remedies or explicitly records every item below.
Final acceptance still depends on the one consolidated owner hardware pass; the
automated remediation is not a substitute for that observation.

## Remediation status

| Finding | Candidate disposition |
| --- | --- |
| F-01 | Fixed: a continuous fractional scheduler preserves 6,400 control boundaries per second across host calls, including blocks smaller than one control interval. The reproducible upstream overlay makes `RampExtractor`, register reacquisition, and X external-clock stabilization elapsed-sample based. Internal/external clock, range/rate, register, and parameter-effect tests run across 32/44.1/48/88.2/96 kHz and 4/24/64/256-frame configurations. |
| F-02 | Fixed in code: target-aware pickup locks each pot whenever T/X or primary/held selection changes and unlocks only when the physical position crosses the destination value. All twelve transitions have executable coverage; physical ergonomics remain in the final owner pass. |
| F-03 | Fixed: T Range uses three equal travel regions with boundary tests. |
| F-04 | Fixed: all seven outputs have appended Add/Replace parameters, defaulting to Add without shifting any existing musical or routing parameter index. Shared-bus behavior is tested. |
| F-05 | Fixed: Deja Vu Length is the exact original 12-choice set. |
| F-06 | Fixed: Y Divider displays the twelve musical ratios while preserving its stored index. |
| F-07 | Decided and recorded: the missing rapid double-press reseed gesture is an approved NT interaction departure, D-NT-06. |
| F-08 | Expanded: deterministic DSP tests now exercise all T models, internal and external rate/range behavior, gate shape/randomness, T/X loop modes and lengths, X/Y distribution controls, relationships, voltage ranges, Y division, reset, scale/register behavior, routes, and Add/Replace output semantics across the engine matrix. The final modular and preset-reload observations remain owner evidence. |

## Authoritative source set

- The current platform source is [disting NT user manual 1.17](https://www.expert-sleepers.co.uk/downloads/manuals/disting_NT_user_manual_1.17.pdf), linked from the [official firmware page](https://www.expert-sleepers.co.uk/distingNTfirmwareupdates.html). Relevant sections are pp. 18-21 (algorithm UI and physical I/O), pp. 29-31 (64-bus flow, output mode, sample rate and block size), p. 40 (preset contents), pp. 43-45 (mappings), and pp. 67-68 (native plug-ins).
- The checked-in API revision is also the current official `main` revision:
  [`f440d20c2a96a9ca43892c7418a1d8185ad47eb5`](https://github.com/expertsleepersltd/distingNT_API/tree/f440d20c2a96a9ca43892c7418a1d8185ad47eb5). Its [API header](https://github.com/expertsleepersltd/distingNT_API/blob/f440d20c2a96a9ca43892c7418a1d8185ad47eb5/include/distingnt/api.h) defines API v13, 12 input + 8 output + 44 aux buses, parameter units and I/O macros, custom-UI event data, soft-takeover setup, preset callbacks, and the 256 x 64 drawing API. The official [custom UI example](https://github.com/expertsleepersltd/distingNT_API/blob/f440d20c2a96a9ca43892c7418a1d8185ad47eb5/examples/gainCustomUI.cpp#L48-L148) demonstrates Add/Replace processing, `hasCustomUi`, `customUi`, `NT_setParameterFromUi`, and `setupUi`.
- The original product contract is the [Marbles printed quickstart](https://pichenettes.github.io/mutable-instruments-documentation/modules/marbles/downloads/marbles_quickstart.pdf) and the more detailed [official web manual](https://pichenettes.github.io/mutable-instruments-documentation/modules/marbles/manual/). The web manual adds the external-CV shift-register behavior, approximately 3 ms acquisition tolerance, scale carving, reseeding gesture, and loop-length explanation.
- The behavioral reference is the final official Mutable Instruments source,
  pinned here at [`08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4`](https://github.com/pichenettes/eurorack/tree/08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4/marbles).

## Required fixes

### F-01: preserve the original processing cadence across NT block sizes and sample rates

The original module processes fixed blocks of five frames at 32 kHz
([`kBlockSize = 5`](https://github.com/pichenettes/eurorack/blob/08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4/marbles/io_buffer.h#L37-L78) and [`kSampleRate = 32000`](https://github.com/pichenettes/eurorack/blob/08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4/marbles/marbles.cc#L58-L66)). Several pieces of the compiled original DSP count calls rather than elapsed samples:

- `NoteFilter` takes about ten `Process()` calls to settle ([source](https://github.com/pichenettes/eurorack/blob/08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4/marbles/note_filter.h#L35-L69)).
- Register-mode reacquisition lasts 20 `OutputChannel::Process()` calls and is explicitly intended to cover roughly 2-3 ms ([source](https://github.com/pichenettes/eurorack/blob/08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4/marbles/random/output_channel.cc#L40-L43), [tracking loop](https://github.com/pichenettes/eurorack/blob/08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4/marbles/random/output_channel.cc#L94-L131)). The web manual likewise says Marbles tolerates up to 3 ms between CV and gate transitions in external processing mode.
- X external-clock stabilization counts 16 `XYGenerator::Process()` calls ([source](https://github.com/pichenettes/eurorack/blob/08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4/marbles/random/x_y_generator.cc#L40-L90)).

The NT defaults to 48 kHz with a 24-frame block and allows both sample rate and
block size to change (NT manual pp. 30-31). The port calls the original
generators and note filter only once per whole host block
([current boundary](https://github.com/thorinside/nt_marbles/blob/17d47c03e5bc0808fb54cba7abde4d9b79a947f2/src/nt_marbles_plugin.cc#L201-L272)); the integration
test uses 64-frame calls ([test](https://github.com/thorinside/nt_marbles/blob/17d47c03e5bc0808fb54cba7abde4d9b79a947f2/tests/plugin_integration_test.cc#L196-L222)).
At the NT default, the three durations above become approximately 5 ms, 10 ms,
and 8 ms instead of about 1.6 ms, 3.1 ms, and 2.5 ms on original hardware.
They drift further with other block settings. The register test waits many
blocks and checks only the eventual pitch, so it cannot detect this error.

There is a second sample-rate dependency in the upstream `RampExtractor`: it
still hard-codes 32,000 when calculating the audio-rate threshold, three-second
timeout, and reset interval ([source](https://github.com/pichenettes/eurorack/blob/08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4/marbles/ramp/ramp_extractor.cc#L56-L87), [later timeout](https://github.com/pichenettes/eurorack/blob/08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4/marbles/ramp/ramp_extractor.cc#L252-L272)). Merely passing `NT_globals.sampleRate` into `Init()` does not make those paths sample-rate independent.

Required acceptance: make these intervals elapsed-sample/time based (in an
NT-boundary patch or a documented upstream overlay), then test internal and
external clocks plus register acquisition at 32, 44.1, 48, 88.2, and 96 kHz and
at the supported minimum/default/maximum NT block sizes.

### F-02: synchronize soft takeover whenever the custom pot target changes

The API says `setupUi()` is called when the algorithm UI first appears so a
custom pot implementation can seed the host's soft-takeover positions
([API contract](https://github.com/expertsleepersltd/distingNT_API/blob/f440d20c2a96a9ca43892c7418a1d8185ad47eb5/include/distingnt/api.h#L470-L485)). The NT manual also makes soft takeover part of normal algorithm editing (pp. 18-19).

Here, each physical pot can target four different parameters: T primary, T held,
X primary, or X held. `setupUi()` supplies only the three currently selected
primary values ([implementation](https://github.com/thorinside/nt_marbles/blob/17d47c03e5bc0808fb54cba7abde4d9b79a947f2/src/nt_marbles_plugin.cc#L356-L367)). Turning
the left encoder changes T/X inside the still-visible custom UI, and holding a
pot changes its target again, without causing the algorithm UI to reappear or
otherwise re-seeding the host pickup value. The callback then writes the raw
absolute pot position into whichever target is current
([implementation](https://github.com/thorinside/nt_marbles/blob/17d47c03e5bc0808fb54cba7abde4d9b79a947f2/src/nt_marbles_plugin.cc#L303-L337)). This can make a target
wait for the wrong pickup point or jump to the physical position. The tests call
`setupUi()` directly after changing workspace, which does not model the host's
callback lifecycle.

Required acceptance: implement target-aware pickup/lock behavior for all twelve
primary/held transitions (or use an API-supported UI structure that re-seeds
pickup), and run the existing AC-008 no-jump check on hardware with deliberately
different source and destination values.

### F-03: fix the three-way T Range pot quantization

The held T Rate pot converts 0-100% using `value * 2 / 100`
([implementation](https://github.com/thorinside/nt_marbles/blob/17d47c03e5bc0808fb54cba7abde4d9b79a947f2/src/nt_marbles_plugin.cc#L330-L336)). Integer division maps
0-49% to `1/4x`, 50-99% to `1x`, and only exactly 100% to `4x`. The original has
three selectable ranges that divide/multiply clock rate by four (Marbles manual,
T generator), and the approved NT surface presents all three as ordinary pot
regions. The test only checks the 100% endpoint.

Required acceptance: use three stable regions across the travel (with sensible
boundary hysteresis if necessary) and test both sides of every transition plus
hardware reachability.

## Integration and fidelity items requiring a decision

### F-04: all outputs silently Replace; no NT Output Mode is exposed

The NT signal-flow contract distinguishes Add and Replace. The manual says
source/instrument algorithms typically add, while both modes are useful in
different routings (pp. 29-30), and API v13 supplies
`NT_PARAMETER_CV_OUTPUT_WITH_MODE`
([macros](https://github.com/expertsleepersltd/distingNT_API/blob/f440d20c2a96a9ca43892c7418a1d8185ad47eb5/include/distingnt/api.h#L267-L289)). The
official custom-UI and monosynth examples implement both modes.

`nt_marbles` declares plain CV outputs and unconditionally assigns every sample
to the selected bus ([parameters](https://github.com/thorinside/nt_marbles/blob/17d47c03e5bc0808fb54cba7abde4d9b79a947f2/src/nt_marbles_plugin.cc#L80-L90),
[writer](https://github.com/thorinside/nt_marbles/blob/17d47c03e5bc0808fb54cba7abde4d9b79a947f2/src/nt_marbles_plugin.cc#L190-L199)). It therefore overwrites an
earlier algorithm and, when two Marbles outputs share a bus, whichever output is
written last wins. Neither behavior is stated in the routing guide or departure
register.

Decision: expose Add/Replace per output (preserving existing indices by appending
new parameters) and test shared-bus behavior, or explicitly approve Replace-only
as a product departure and document its chain-order consequences.

### F-05: Deja Vu Length admits four lengths the original panel cannot select

The final source's physical quantizer selects exactly `1, 2, 3, 4, 5, 6, 7, 8,
10, 12, 14, 16` ([table](https://github.com/pichenettes/eurorack/blob/08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4/marbles/marbles.cc#L196-L209)). The port exposes every integer 1-16
([parameter](https://github.com/thorinside/nt_marbles/blob/17d47c03e5bc0808fb54cba7abde4d9b79a947f2/src/nt_marbles_plugin.cc#L73-L79)), adding 9, 11, 13, and 15.
Those values are accepted by the generator but are an undocumented extension,
not source fidelity.

Decision: make the parameter a 12-choice enum with the original values, or add
the extra lengths to `docs/departures.md` as an intentional extension and test
their preset compatibility.

### F-06: Y Divider is displayed as an implementation index, not a ratio

The original Y control is a musical division of X2 from 1/64 through 1
([web manual, Y generator](https://pichenettes.github.io/mutable-instruments-documentation/modules/marbles/manual/#y-generator)). The port has the correct ratio table but declares the parameter as `kNT_unitNone` with values 0-11
([implementation](https://github.com/thorinside/nt_marbles/blob/17d47c03e5bc0808fb54cba7abde4d9b79a947f2/src/nt_marbles_plugin.cc#L44-L47),
[parameter](https://github.com/thorinside/nt_marbles/blob/17d47c03e5bc0808fb54cba7abde4d9b79a947f2/src/nt_marbles_plugin.cc#L78-L80)). In the standard NT parameter
view and mappings, players therefore see an opaque index rather than `1/64`,
`1/48`, ... `1/1`.

Required UX fix: provide enum/custom strings for the exact ratios and assert all
twelve displayed values. This does not require changing the stored index.

### F-07: original reseeding is not represented or registered as a departure

The official web manual's tips describe rapid double-pressing the Deja Vu
buttons to reseed a loop. The approved NT mapping instead makes each right
encoder press cycle Off -> Amount -> Super-lock
([implementation](https://github.com/thorinside/nt_marbles/blob/17d47c03e5bc0808fb54cba7abde4d9b79a947f2/src/nt_marbles_plugin.cc#L348-L353)). A double press changes
mode twice; it is not a reseed command and does not restore the starting mode.

Decision: expose a deliberate reseed gesture/trigger parameter, or document the
missing performance gesture as a behavioral departure. This is distinct from
D-NT-06, which already documents the amount deadband and scale recording.

### F-08: behavior tests do not yet prove the full parameter surface

The current integration test proves route activity, two scale tables, eventual
register pitch difference, draw calls, gestures, and host-owned parameter
restoration. It does not establish output behavior for all six T models, the
three T ranges, internal BPM endpoints, external clock division ratios, jitter,
gate length/randomness, all X relationships and ranges, Deja Vu amount/lock/
length behavior, Y divider ratios, reset alignment, shared output buses, or the
external-CV shift-register ordering described by the web manual. Several Python
tests assert that source text exists rather than exercising behavior.

Required acceptance: add deterministic effect tests for every musical parameter
and for the paired interactions above. Treat native tests, NT engine-configuration
tests, and physical modular observations as separate evidence.

## Source/manual ambiguities to record, not silently “fix”

- Both first-party Marbles manuals say Y defaults to X2 divided by 16. The final
  source initializes `y_divider` to 128
  ([settings](https://github.com/pichenettes/eurorack/blob/08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4/marbles/settings.cc#L182-L199)); its 12-entry lookup selects index 6, or 1/8
  ([orchestration](https://github.com/pichenettes/eurorack/blob/08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4/marbles/marbles.cc#L181-L194)). The port's 1/8 default follows final source, not the manuals. Keep 1/8 for source fidelity, but document this explicit source/manual decision.
- The printed quickstart's Tilt illustration/text reverses X1 and X3. The online
  manual and final `XYGenerator` agree that X1 responds oppositely, X2 stays
  neutral, and X3 follows the controls
  ([source](https://github.com/pichenettes/eurorack/blob/08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4/marbles/random/x_y_generator.cc#L151-L166)). The port follows source and the web manual, so no code change is indicated.

## Already covered or explicitly departed

- T Rate is no longer presented as semitones. The original manual calls it clock
  rate, with 120 BPM at center; with external clock it becomes division/
  multiplication. The source's -60..+60 coordinate is only an internal
  logarithmic representation
  ([input mapping](https://github.com/pichenettes/eurorack/blob/08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4/marbles/cv_reader.cc#L41-L59),
  [clock calculation](https://github.com/pichenettes/eurorack/blob/08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4/marbles/random/t_generator.cc#L323-L357)). The current 0-100% physical-position display is truthful. BPM when internally clocked and the active division ratio when externally clocked would be more informative, but that is a UX improvement rather than a remaining semitone defect.
- API v13, all 64 buses, the `.o` install layout under
  `/programs/plug-ins`, automatic plug-in loading with presets, 256 x 64 drawing,
  and the corrected encoder event source (`encoders[]`, not `controls`) all match
  current Expert Sleepers documentation.
- T1/T2/T3, X1/X2/X3, and Y are all represented. The guide correctly makes clear
  that NT outputs 7 and 8 are used by defaults; the NT manual notes those are
  rear-header outputs rather than front sockets (pp. 20-21).
- The exact six final factory tables are loaded. The deliberately omitted scale
  recording/reset workflow, calibration/color presentation, physical CV
  attenuverters, self-patch detection, gate-driver delay, and explicit-reset
  gesture are already recorded in `docs/departures.md`.
- The 75% X Steps new-instance default is an explicit NT starting-value departure
  and now has an audible scale-effect test.
- Host-owned declared parameters and routes are correctly the normal preset
  surface. Not serializing the evolving random sequence starts a fresh valid
  sequence on construction, matching the absence of random-sequence persistence
  in original hardware state; this is already stated in the control-surface guide.
