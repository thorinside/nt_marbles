# Behavioral departures

This register compares `src/nt_marbles_plugin.cc` with the pinned upstream
`marbles/marbles.cc` orchestration at
`08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4`. The generator implementation
files themselves remain upstream source; the entries below are explicit NT
boundary adaptations, not hidden changes to that source.

| ID | Original behavior | NT adaptation and reason | Verification method |
| --- | --- | --- | --- |
| D-NT-01 | Marbles reads calibrated panel pots and attenuverted CV inputs through its ADC/CV-reader layer. | NT primary, secondary, and Deja Vu values are normal host-mappable parameters, scaled at the boundary. The physical calibration and per-input attenuverter layer is not reproduced because the NT host owns parameter/CV mapping. | Inspect `step()` setters and the complete parameter table in `docs/control-surface.md`; run `make test`. |
| D-NT-02 | In register mode, Marbles captures the filtered live X CV input; its value is also affected by the physical X-spread attenuverter. | `X Register CV` is an independently routable NT signal input. It converts −5…+5 V to the original 0…1 range and uses the pinned Marbles note filter. Ordinary X Spread modulation remains host-mapped, replacing the panel's dedicated attenuverter. | Inspect `kXRegisterCvRoute`, `RegisterValueFromVolts`, `NoteFilter`, and `x.register_value` in `step()`; run `tests/host_boundary_test.cc` and `tests/routing_contract_test.py`. |
| D-NT-03 | The physical `explicit_reset` preference enables section resets from the Jitter and Steps hidden-gate inputs. | A rising NT `Reset` signal resets both generators directly. This is required by the approved actual-reset route and is explicit rather than retaining a hidden-panel gesture. | Inspect `didReset` and both `Process` calls in `step()`; run `tests/routing_contract_test.py`. |
| D-NT-04 | The physical X clock input can self-patch-detect individual T outputs, selecting T1, T2, or T3 as its internal source. | `X Clock` is independently routable. When disconnected it uses Marbles' `T1_T2_T3` normalled source; when connected it is external. Individual self-patch source detection is not reproduced because NT routes have no equivalent self-patch observation. | Inspect the `xClockSource` branch in `step()` and the routing table; run `tests/routing_contract_test.py`. |
| D-NT-05 | Hardware gate outputs are delayed by the STM gate-output driver, and Marbles starts its RNG with seed `1`. | NT writes immediate 0 V/5 V gate signals because the NT host provides float buses rather than the original STM gate driver. The original seed `1` is retained. | Compare the upstream output loop and `random_generator.Init(1)` with `step()`/`construct()`; run `make inspect`. |
| D-NT-06 | Marbles persists calibration, six editable factory scales, `color_blind`, and `explicit_reset` in its hardware settings UI; the Deja Vu knob also has a physical center deadband. | NT loads the exact six pinned factory scales and exposes their selection as host state, but does not reproduce scale recording, hardware calibration, or the hardware reset preference. Explicit text replaces LEDs and exposes **Super-lock** as a named state. | Compare `settings.cc`, `settings.h`, `marbles.cc`, and the parameter/control tables; run `tests/control_surface_test.py`, `tests/verification_contract_test.py`, and `tests/persistence_contract_test.py`. |

No compatibility patch is currently required. If one becomes necessary, add its
patch under `patches/eurorack/` and add a row here before release. `make
verify-overlay` must leave `third_party/eurorack` clean after applying and
removing that patch layer.
