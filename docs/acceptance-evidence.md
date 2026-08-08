# nt_marbles acceptance evidence

This matrix tracks the original approved specification and its full AC-001
through AC-021 target. Automated evidence comes from the
actual plugin factory/callback integration harness, the small host-boundary
test, supplemental contract checks, ARM inspection, and repository evidence.
The exact tested commit and object checksum are generated into the release
bundle by `make package`.

Hardware validation has produced three negative observations: the original object
had a sparse, apparently inactive interface, and the first recovery candidate
did not respond to real encoder events while its unquantized X Steps midpoint
made factory-scale behavior difficult to hear. The next pass exposed that T
Rate's internal exponential coordinate was incorrectly labelled as semitones
and that T Range reused the unrelated X/Y voltage-range labels. These findings
reopened their automated slices. The encoder event contract, live numeric
display, visible active scale, quantized new-instance X Steps value, normalized
T Rate presentation, and distinct clock-range labels now have executable
regression coverage. The official-manual audit then found and corrected engine
cadence/sample-rate handling, stale soft takeover after target changes, T Range
partitioning, loop-length choices, Y ratio presentation, and Add/Replace bus
semantics. Final code candidate
`b72ccb93b5b4377718fe9db491230d7567029a9b` passed GitHub Actions run
`31281201514`; its CI object SHA-256 is
`c5e4b7320d17b6cbe59e4b5aa4812764f9f87c4bc0acc460b8544b241b0c67ba`.
The NT required a reboot after an earlier object introduced an unsupported
`__aeabi_ldivmod` helper; the final build rejects all such helpers. After the
reboot, nt_helper reported live GUID `ThMB`, all 42 host parameters, and the
custom T screen. The owner then completed the single consolidated hardware
session with three oscillators and explicitly replied `pass` to the complete
acceptance checklist on 2026-08-08.

| Criterion | Current disposition | Durable evidence | Final owner observation |
| --- | --- | --- | --- |
| AC-001 | Automated pass | README and `verification_contract_test.py` identify and verify Eurorack revision `08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4`. | Not required. |
| AC-002 | Automated pass | `src/nt_marbles_plugin.cc` is the NT boundary; pinned generator sources are compiled directly by the Makefile; host conversion helpers are isolated in `src/nt_marbles_host_boundary.h`. | Not required. |
| AC-003 | Automated + owner pass | `docs/departures.md` records D-NT-01 through D-NT-08 with original behavior, NT adaptation/reason, and verification method, including manual/source ambiguity and omitted reseed gesture. | Owner reported no unexpected musical departure in the final pass. |
| AC-004 | Automated pass | Recursive submodules, exact revisions, staged compatibility overlay, README reproduction instructions, and CI path are checked in; the engine matrix covers all five documented sample rates and the scheduler proves 6,400 control boundaries per second for every tested block size. | Not required. |
| AC-005 | Automated + owner pass | Real draw/custom-UI callbacks prove visible T/X mode, live model/relationship and Deja Vu state, primary labels, held alternatives, overview, and contextual help. | Owner accepted the corrected live screens as understandable. |
| AC-006 | Owner pass | The approved rendered prototype is `docs/control-surface-prototype.svg`; executable draw evidence proves its frame, divider, hierarchy, labels, and states. Direct nt_helper MCP capture proves the final object executes the custom UI. | Owner explicitly passed final mapping, hierarchy, and interaction on hardware. |
| AC-007 | Automated pass | `make verify-overlay` applies/removes the sample-rate compatibility patch, asserts the pinned checkout is clean, and prepares the same staged source compiled by normal builds. | Not required. |
| AC-008 | Automated + owner pass | Real custom-UI tests cover every primary and held target, target-aware pickup crossings, T Range boundaries, non-destructive plain presses, and bounded 256 x 64 drawing. | Owner passed physical holds, pickup, presses, and lack of jumps. |
| AC-009 | Automated + owner pass | The real claimed-control mask excludes Buttons 1/2/4, includes Button 3, and executable tests prove contextual T/X help and no mutation from host buttons. | Owner passed normal host navigation and help behavior. |
| AC-010 | Automated + owner pass | Real callback tests cover FR-014 through FR-018: paired T/X pot layers, both left-encoder actions, both right-encoder actions, and explicit named states. | Owner approved Faceplate-led concept 6 on hardware. |
| AC-011 | Automated + owner pass | The original-parameter table covers every ADC/state value; exact tests cover the original 12 loop lengths, all Y-divider labels, scales, defaults, and the complete 23-parameter musical surface. | Owner passed the final musical-control spot checks. |
| AC-012 | Automated + owner pass | The original-output table covers T1/T2/T3/X1/X2/X3/Y; actual processing observes every channel and proves per-output Add/Replace behavior. | Owner passed representative physical rhythm/pitch output and Add/Replace checks with three oscillators. |
| AC-013 | Automated + owner pass | The signal-routing table traces all four inputs and seven outputs; the integration harness drives actual buses through `step()` and catches interleaving and T2 overwrite defects. | Owner passed the final physical clock/reset/output patch. |
| AC-014 | Automated pass | The Makefile compiles the pinned TGenerator, XYGenerator, OutputChannel, Quantizer, distributions, lag, ramp, resources, and stmlib units; exact scale data is compared to upstream. | Not required. |
| AC-015 | Automated pass | Required disting NT and upstream headers are pinned; README documents Makefile and `.github/workflows/build.yml`; CI runs native tests, overlay verification, ARM packaging, rejects unsupported `__aeabi_*` runtime helpers, and uploads the artifact. | Not required. |
| AC-016 | Automated pass | Factory tests assert GUID `NT_MULTICHAR('T','h','M','B')`; README records the `Th` thorinside namespace. | Not required. |
| AC-017 | Automated + owner pass | Every declared CV input/output has minimum 0; actual callback tests prove route 0 performs no read/write and invalid routes are also safe. | Owner accepted final routing/disconnection behavior. |
| AC-018 | Automated + owner pass | Real factory defaults assert independent T Clock=1, X Clock=1, Reset=2; external and T-normalled clock paths both execute; all inputs can be independently set to 0 or any valid bus. | Owner passed final clock and reset patching. |
| AC-019 | Automated + owner pass | Real factory defaults assert T1=15, X1=16, T2=17, X2=18, T3=19, X3=20 (physical outputs 3 through 8); processing proves outputs 1/2 remain untouched. | Owner passed representative physical T1/T2/T3 and X1/X2/X3 outputs. |
| AC-020 | Automated + owner pass | Y is a declared independent route with default 0 and full valid bus range; actual processing proves distinct Y activity when routed. | Owner accepted the final output/routing surface. |
| AC-021 | Automated + owner pass | The complete 41-value surface is 23 musical parameters, four inputs, seven outputs, and seven output modes in host-managed `v`; no duplicate route state or substitution exists. | Owner passed preset save/reload and restoration of representative values and routing. |

## Automated commands

- `make test`
- `make verify-overlay`
- `make inspect`
- `make package`

Owner acceptance is recorded from an explicit final `pass`, not inferred from
readiness.
