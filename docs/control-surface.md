# T and X control surface

This is the approved Faceplate-led control contract for the NT interface. It is
implemented in `src/nt_marbles_plugin.cc` as declared NT parameters, so the
host saves/restores these values with presets. The DSP adapter has not yet been
connected; this document deliberately makes no claim that changing a control
currently changes DSP output.

## Workspaces and gestures

| Workspace | pot turn | hold the same pot, then turn |
| --- | --- | --- |
| T rhythm | Rate · Bias · Jitter | Range · Gate Length · Gate Randomness |
| X pitch | Spread · Bias · Steps | Y Spread · Y Bias · Y Steps |

A plain pot press has no action. The left encoder selects T or X and its press
toggles the overview. The right encoder selects **T Model** (Complementary,
Clusters, Drums, Independent, Divider, Three states, Markov) or **X
Relationship** (Identical, Bump, Tilt); its press cycles the relevant Deja Vu
state: **Off**, **Amount**, **Super-lock**. Button 3, while held, presents a
two-row contextual gesture reminder. Buttons 1, 2, and 4 are not claimed by
the plug-in and remain NT navigation controls.

The custom display has only text at fixed coordinates within the 256 × 64 NT
screen and uses the API default cyan shade. Its workspace screen gives the
three primary names and their hold alternatives; the overview names encoder
roles and help.

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

`State::color_blind` is a physical-panel LED presentation preference, not a
musical DSP parameter. Explicit NT labels replace that panel behavior. The
physical module's `explicit_reset` setting is represented in this port by the
future reset signal route, rather than by a stored knob parameter.

## Review required

[`control-surface-prototype.svg`](control-surface-prototype.svg) renders the T,
X, overview, and help screens at their actual 256 × 64 dimensions. The rendered interaction is ready for an owner review, but no owner approval
has been recorded. Review the T screen, X screen, overview, and held Button 3
help on a 256 × 64 target/emulator; then confirm that the names, hierarchy, and
hold gesture match the approved concept.
