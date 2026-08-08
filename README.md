# nt_marbles

A disting NT port of Mutable Instruments Marbles. It compiles the pinned
original generators, loads the original factory scales, and adapts their
interleaved signals to a tested disting NT host boundary and the approved T/X
control surface. See [the control-surface guide](docs/control-surface.md).

## Provenance and licensing

The Marbles implementation reference is the read-only Git submodule
[`third_party/eurorack`](third_party/eurorack), pinned to:

- repository: <https://github.com/pichenettes/eurorack>
- revision: `08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4`

Its nested `stmlib` dependency is pinned by that repository at
`e3bd7c9cc00e4364166f9905c0509b6ffd0535ec`. The original source's license
notices remain with that source and must be retained in any distribution.

The disting NT API headers are a separate read-only submodule:

- repository: <https://github.com/expertsleepersltd/distingNT_API>
- revision: `f440d20c2a96a9ca43892c7418a1d8185ad47eb5` (API v13)

`src/nt_marbles_plugin.cc` is the NT-only adapter and declares the T/X control
surface as host-managed parameters. Its small pure conversion helpers live in
`src/nt_marbles_host_boundary.h`; it compiles original generator units straight
from a generated, patched staging copy of `third_party/eurorack`, as listed in
the Makefile. The immutable factory
scale table in `src/marbles_factory_scales.h` is automatically compared with
the pinned upstream settings data. The pinned checkout itself remains clean.

The plugin factory GUID is `NT_MULTICHAR('T', 'h', 'M', 'B')`; `Th` reserves
the owner's thorinside namespace.

## Install

nt_marbles requires disting NT firmware 1.15.0 or later (plug-in API v13).
Download `nt_marbles-plugin.zip` from the latest GitHub Release and extract it
at the root of the disting NT MicroSD card. The plug-in object will be installed
at:

```text
/programs/plug-ins/nt_marbles.o
```

Rescan plug-ins or reboot the module, then add **Marbles** to a preset. New
instances route T1/X1/T2/X2/T3/X3 to physical outputs 3 through 8; Y starts
disconnected. The complete controls, routing defaults, and NT adaptations are
documented in [the control-surface guide](docs/control-surface.md) and
[behavioral departure register](docs/departures.md).

## Reproduce a build

```sh
git clone --recurse-submodules <this repository>
cd nt_marbles
git submodule update --init --recursive
make ARM_CXX=arm-none-eabi-g++ inspect
```

The output is `plugins/nt_marbles.o`, an ARM relocatable disting NT plugin
object. `make inspect` confirms that it is an ARM ELF object and exports
`pluginEntry`.

`make package` creates `nt_marbles-plugin.zip` in the installable MicroSD
layout, includes the exact source commit, object checksum, and third-party
notices, and writes `nt_marbles-plugin.zip.sha256` beside it.

## Compatibility overlay

Compatibility changes are deliberately outside the submodule in ordered
`patches/eurorack/*.patch` files. The current overlay makes the upstream clock
extractor and remaining call-count timing windows sample-rate aware. `make` runs
`scripts/prepare-eurorack-overlay.sh` and compiles `build/eurorack-overlay`,
leaving the pinned checkout untouched. These verification commands are safe
when the upstream checkout is clean:

```sh
./scripts/apply-eurorack-patches.sh
./scripts/remove-eurorack-patches.sh
# or both plus a clean-check:
make verify-overlay
```

The verification path uses `git apply` and reverse `git apply`; neither command
commits to or changes the pinned submodule after removal. `make test` also runs
the real factory across all five documented NT sample rates and representative
4-, 24-, 64-, and 256-frame blocks. A continuous fractional control scheduler
also proves the original 6,400 control boundaries per second are independent of
the host's block partitioning. ARM inspection additionally rejects unresolved
`__aeabi_*` runtime helpers, which the disting NT plug-in loader does not supply.

## Build automation

`.github/workflows/build.yml` is the Linux CI path. It checks out recursive
submodules, builds and inspects the ARM object, and uploads the installable
MicroSD-layout archive. Pushes of `v*` tags additionally publish that verified
archive and its SHA-256 file as a GitHub Release.
