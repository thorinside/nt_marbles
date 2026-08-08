# T and X control surface

This is the approved Faceplate-led control contract for the NT interface. It is
implemented in `src/nt_marbles_plugin.cc` as declared NT parameters, so the
host saves/restores these values with presets. The NT boundary scales these
parameters into the pinned Marbles generators during each audio step.

## Workspaces and gestures

| Workspace | pot turn | hold the same pot, then turn |
| --- | --- | --- |
| T rhythm | Rate · Bias · Jitter | Range · Gate Length · Gate Randomness |
| X pitch | Spread · Bias · Steps | Y Spread · Y Bias · Y Steps |

A plain pot press has no action. The left encoder selects T or X and its press
toggles the overview. The right encoder selects **T Model** (Complementary,
Clusters, Drums, Independent, Divider, Three states) or **X
Relationship** (Identical, Bump, Tilt); its press cycles the relevant Deja Vu
state: **Off**, **Amount**, **Super-lock**. Button 3, while held, presents a
two-row contextual gesture reminder. Buttons 1, 2, and 4 are not claimed by
the plug-in and remain NT navigation controls.

Whenever a pot changes between a T primary, T held, X primary, or X held
target, the custom UI locks that pot until its physical position crosses the
new target's value. This target-aware pickup extends the host's initial
`setupUi()` soft takeover across all twelve live target transitions, so a
workspace or hold change cannot jump the newly selected parameter.

The custom display uses text at fixed coordinates plus a dim cyan frame and
header divider within the 256 × 64 NT screen. The T header shows the current
model and Deja Vu state. The X header shows the current relationship, active
factory scale, and Deja Vu state. Both parameter rows show live numeric values,
so every pot movement produces visible feedback while retaining the primary and
held-role hierarchy. The overview names encoder roles and help.

## Original-parameter coverage

The table is indexed against the original `marbles/drivers/adc.h` and
`marbles/settings.h` definitions at the pinned upstream revision. All values
below are declared normal NT parameters and therefore are host-mappable.

| Original Marbles value | NT representation |
| --- | --- |
| `ADC_CHANNEL_T_RATE` | T Rate |
| `ADC_CHANNEL_T_BIAS` | T Bias |
| `ADC_CHANNEL_T_JITTER` | T Jitter |
| `ADC_CHANNEL_X_SPREAD` | X Spread |
| `ADC_CHANNEL_X_BIAS` | X Bias |
| `ADC_CHANNEL_X_STEPS` | X Steps |
| `ADC_CHANNEL_DEJA_VU_AMOUNT` | Deja Vu Amount |
| `ADC_CHANNEL_DEJA_VU_LENGTH` | Deja Vu Length |
| `State::t_deja_vu` | T Deja Vu (Off / Amount / Super-lock) |
| `State::t_model` | T Model |
| `State::t_range` | T Range |
| `State::t_pulse_width_mean` | T Gate Length |
| `State::t_pulse_width_std` | T Gate Randomness |
| `State::x_deja_vu` | X Deja Vu (Off / Amount / Super-lock) |
| `State::x_control_mode` | X Relationship |
| `State::x_register_mode` | X Register |
| `State::x_range` | X Range |
| `State::x_scale` | X Scale |
| `State::y_spread` | Y Spread |
| `State::y_bias` | Y Bias |
| `State::y_steps` | Y Steps |
| `State::y_divider` | Y Divider |
| `State::y_range` | Y Range |

`T Rate` is shown as the physical knob's normalized 0…100% position, with 50%
at the center. Internally that position maps to the original exponential
−60…+60 rate coordinate; it is not presented as semitones because it controls
clock speed or external-clock division/multiplication, not pitch. `T Range`
uses three equal pot regions for the original 1/4x, 1x, and 4x clock ranges.
`Deja Vu Length` exposes only the original choices 1–8, 10, 12, 14, and 16.
`Y Divider` displays its twelve musical ratios from 1/64 through 1/1 rather
than the stored 0–11 index. The six `X Scale` choices load
the exact pinned factory tables:
C major, C minor, Pentatonic, Pelog, Raag Bhairav That, and Raag Shri. Persisted
defaults mirror upstream `Settings::Init`: X and Y Range are Full, Y Steps is
0%, and Y Divider is 1/8. Both official manuals describe that Y default as
1/16, but the final pinned source's `128 * 12 >> 8` lookup selects 1/8; this
port follows the final source and records the discrepancy as D-NT-08. The
upstream-only Markov enum is not exposed because
the original panel and persisted state constrain T Model to the first six
choices.

The original X Steps control is a physical pot and therefore has no stored
power-on value in `Settings::Init`. A new NT instance starts X Steps at 75%,
above Marbles' unquantized midpoint, so the named factory scale is immediately
audible. Turning Steps down through 50% returns to the original continuous
random-voltage behavior. This NT-specific starting value is recorded as
departure D-NT-07.

`State::color_blind` is a physical-panel LED presentation preference, not a
musical DSP parameter. Explicit NT labels replace that panel behavior. The
physical module's `explicit_reset` setting is adapted to the actual `Reset`
signal route rather than a stored knob parameter. Its intentionally different
NT semantics are recorded in [the departure register](departures.md).

## Signal routing and output coverage

All routing values are declared NT I/O parameters and are therefore saved and
restored by the host with a preset. `0` is the minimum for every selector and
means **Disconnected**: the boundary neither reads that input nor writes that
output. It never replaces a disconnected or unavailable saved route with a
different bus.

NT bus numbers 1–12 are inputs. Physical NT output *N* is bus `12 + N`, so the
new-instance routes below leave physical outputs 1 and 2 unused.

| Route | Type | Default | DSP boundary trace |
| --- | --- | ---: | --- |
| T Clock | signal input | input 1 | Gate transitions become `stmlib::GateFlags` and drive `TGenerator::Process`. |
| X Clock | signal input | input 1 | Independently converted to gate transitions and drives `XYGenerator::Process` as its external clock. When disconnected, X uses Marbles’ internal T-normalled clock relationship. |
| Reset | signal input | input 2 | A rising transition resets both generator calls for that processing block. |
| X Register CV | signal input | disconnected (0) | When X Register is On, −5…+5 V is clamped to the original 0…1 note range and passed through the pinned Marbles note filter before `XYGenerator::Process`. |
| T1 Output | CV output | output 3 (bus 15) | Marbles T gate channel 1 → 0 V / 5 V NT signal. |
| X1 Output | CV output | output 4 (bus 16) | Marbles X voltage channel 1 → NT CV signal. |
| T2 Output | CV output | output 5 (bus 17) | Marbles master-ramp T gate → 0 V / 5 V NT signal. |
| X2 Output | CV output | output 6 (bus 18) | Marbles X voltage channel 2 → NT CV signal. |
| T3 Output | CV output | output 7 (bus 19) | Marbles T gate channel 2 → 0 V / 5 V NT signal. |
| X3 Output | CV output | output 8 (bus 20) | Marbles X voltage channel 3 → NT CV signal. |
| Y Output | CV output | disconnected (0) | Marbles Y voltage channel → independently selectable NT CV signal. |

Each of the seven outputs also has a normal NT **Add/Replace** mode parameter,
defaulting to Add. Add sums Marbles onto the existing bus; Replace overwrites
that bus. This makes shared-bus and algorithm-chain behavior explicit instead
of silently replacing whatever an earlier algorithm wrote.

The primary and held controls in the preceding tables are adjustable
host-mappable parameters. `step()` maps the displayed T Rate knob position to
the original exponential rate coordinate and maps remaining percentages and
named states to the corresponding original `TGenerator` and `XYGenerator`
setters and `GroupSettings`. T1/T3 are deinterleaved from the two-channel gate
frames; X1/X2/X3/Y are deinterleaved from the four-channel voltage frames. No
route is stored by plug-in serialization (`serialise` and `deserialise` are
null).

The original generators are called at the same elapsed-time control cadence as
their five-frame/32 kHz hardware blocks. A continuous fractional scheduler
crosses host-call boundaries (for example, alternating 7 and 8 frames at 48
kHz) instead of restarting at each NT block. A removable overlay also converts
the clock extractor, register reacquisition, and X-clock stabilization timing
to elapsed samples. The engine matrix executes the real factory at 32, 44.1,
48, 88.2, and 96 kHz and at 4-, 24-, 64-, and 256-frame host blocks; a boundary
test proves every configuration preserves 6,400 control intervals per second.

## Host-native preset restoration

The declared `kParameters` array is the complete preset surface: 23 musical
values, four signal-input routes, seven output routes, and seven output modes.
The NT API owns
`_NT_algorithm::v`, so normal host preset save/reload restores those declared
values and routes; nt_marbles keeps no duplicate parameter or routing store.

There is currently no demonstrated continuity requirement for serializing
internal state. On construction, the original-derived random generator is
initialized to its normal valid starting state and both original generators are
initialized from it. Reloading therefore starts a fresh, valid Marbles random
sequence rather than attempting an undocumented partial random-state restore.
Both custom JSON callbacks remain null until a concrete continuity requirement
is established and documented.

A route value is never rewritten by the plug-in. At the DSP boundary, 0 and an
out-of-range value safely disconnect that route; valid saved values are read or
written exactly as supplied. The NT host governs unavailable saved routes, and
the plug-in does not substitute a different route.

### Original output coverage

| Original Marbles output function | Usable NT representation |
| --- | --- |
| T1 rhythm/trigger | `T1 Output`, independently routable; default physical output 3. |
| T2 rhythm/trigger | `T2 Output`, independently routable; default physical output 5. |
| T3 rhythm/trigger | `T3 Output`, independently routable; default physical output 7. |
| X1 random CV | `X1 Output`, independently routable; default physical output 4. |
| X2 random CV | `X2 Output`, independently routable; default physical output 6. |
| X3 random CV | `X3 Output`, independently routable; default physical output 8. |
| Y related random CV | `Y Output`, independently routable; default disconnected. |

## Validation status

[`control-surface-prototype.svg`](control-surface-prototype.svg) renders the T,
X, overview, and help screens at their actual 256 × 64 dimensions, including
the current model/relationship and Deja Vu labels. Automated tests exercise the
T screen, X screen, overview, all six primary/held pot gestures, plain pot
presses, Button 3 help, and unclaimed Buttons 1, 2, and 4. Final owner validation
on physical hardware passed the mapping, hierarchy, pickup behavior, navigation,
rhythm/pitch output with three oscillators, and preset restoration on
2026-08-08. See [the acceptance evidence](acceptance-evidence.md) for the exact
automated and physical evidence boundaries.
