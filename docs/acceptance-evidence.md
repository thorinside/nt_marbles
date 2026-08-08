# nt_marbles acceptance evidence

This matrix tracks the original approved Spec `fa577bb1-5fbf-49c2-b064-b241fb85a18e`
and its full AC-001 through AC-021 target. Automated evidence comes from the
actual plugin factory/callback integration harness, the small host-boundary
test, supplemental contract checks, ARM inspection, and repository evidence.
The exact tested commit and object checksum are generated into the release
bundle by `make package`.

The prior hardware observation is negative evidence for the old object, not
acceptance of this corrected build. The owner fields below remain pending until
one final validation session with the exact packaged object.

| Criterion | Current disposition | Durable evidence | Final owner observation |
| --- | --- | --- | --- |
| AC-001 | Automated pass | README and `verification_contract_test.py` identify and verify Eurorack revision `08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4`. | Not required. |
| AC-002 | Automated pass | `src/nt_marbles_plugin.cc` is the NT boundary; pinned generator sources are compiled directly by the Makefile; host conversion helpers are isolated in `src/nt_marbles_host_boundary.h`. | Not required. |
| AC-003 | Automated pass | `docs/departures.md` records D-NT-01 through D-NT-06 with original behavior, NT adaptation/reason, and verification method. | Confirm no unexpected musical departure during final use. |
| AC-004 | Automated pass | Recursive submodules, exact revisions, Makefile source list, overlay scripts, README reproduction instructions, and CI path are checked in. | Not required. |
| AC-005 | Automated pass | Real draw/custom-UI callbacks prove visible T/X mode, live model/relationship and Deja Vu state, primary labels, held alternatives, overview, and contextual help. | Confirm the corrected live screens are understandable. |
| AC-006 | Pending owner | The approved rendered prototype is `docs/control-surface-prototype.svg`; executable draw evidence proves its frame, divider, hierarchy, labels, and states. | Approve or reject mapping, hierarchy, and interaction on the exact final object. |
| AC-007 | Automated pass | `make verify-overlay` applies/removes the empty ordered overlay and asserts the pinned Eurorack checkout is clean. | Not required. |
| AC-008 | Automated pass | Real custom-UI tests cover every primary and same-pot held alternative; unchanged value snapshots prove plain presses are non-destructive; shape coordinates are bounded to 256 x 64. | Confirm physical hold-and-turn ergonomics and lack of jumps. |
| AC-009 | Automated pass | The real claimed-control mask excludes Buttons 1/2/4, includes Button 3, and executable tests prove contextual T/X help and no mutation from host buttons. | Confirm normal host navigation on hardware. |
| AC-010 | Automated match; owner pending | Real callback tests cover FR-014 through FR-018: paired T/X pot layers, both left-encoder actions, both right-encoder actions, and explicit named states. | Approve Faceplate-led concept 6 on hardware. |
| AC-011 | Automated pass | The original-parameter table in `docs/control-surface.md` covers every ADC/state value; persistence and verification tests check the complete 23-parameter musical surface. | Spot-check musical behavior in final session. |
| AC-012 | Automated pass | The original-output table covers T1/T2/T3/X1/X2/X3/Y; actual processing observes every channel through its independently configured route. | Confirm physical output activity in final session. |
| AC-013 | Automated pass | The signal-routing table traces all four inputs and seven outputs; the integration harness drives actual buses through `step()` and catches interleaving and T2 overwrite defects. | Confirm the final patch on physical inputs/outputs. |
| AC-014 | Automated pass | The Makefile compiles the pinned TGenerator, XYGenerator, OutputChannel, Quantizer, distributions, lag, ramp, resources, and stmlib units; exact scale data is compared to upstream. | Not required. |
| AC-015 | Automated pass | Required disting NT and upstream headers are pinned; README documents Makefile and `.github/workflows/build.yml`; CI runs native tests, overlay verification, ARM packaging, and artifact upload. | Not required. |
| AC-016 | Automated pass | Factory tests assert GUID `NT_MULTICHAR('T','h','M','B')`; README records the `Th` thorinside namespace. | Not required. |
| AC-017 | Automated pass | Every declared CV input/output has minimum 0; actual callback tests prove route 0 performs no read/write and invalid routes are also safe. | Confirm representative disconnection on hardware. |
| AC-018 | Automated pass | Real factory defaults assert independent T Clock=1, X Clock=1, Reset=2; external and T-normalled clock paths both execute; all inputs can be independently set to 0 or any valid bus. | Confirm clock and reset patching in final session. |
| AC-019 | Automated pass | Real factory defaults assert T1=15, X1=16, T2=17, X2=18, T3=19, X3=20 (physical outputs 3 through 8); processing proves outputs 1/2 remain untouched. | Confirm default physical outputs in final session. |
| AC-020 | Automated pass | Y is a declared independent route with default 0 and full valid bus range; actual processing proves distinct Y activity when routed. | Confirm a chosen Y route on hardware. |
| AC-021 | Automated contract pass; owner reload pending | The complete surface is 23 musical parameters, four inputs, and seven outputs in host-managed `v`; no plugin serialisation, duplicate route state, route rewriting, or fallback substitution exists. | Save/reload the exact final preset and confirm every sampled parameter/route restores. |

## Automated commands

- `make test`
- `make verify-overlay`
- `make inspect`
- `make package`

## Delivery lifecycle

- Recovery delivery map: `4d3ccc47-a0e1-4c75-9088-c051531fa8bd`
- Final owner ticket: `c4e7a0f5-1614-4ac9-9e29-5b18398a1bfd`
- Existing review root to resume: `9dfdca44-3366-40dc-b5a6-0afe70a0e309`
- Existing Portal execution: `f2c7b4f2-5b88-410f-8a59-e8c6628eec09`

The delivery map and owner ticket remain open while the final observation
column is pending. Readiness is not owner acceptance.
