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
from `third_party/eurorack`, as listed in the Makefile. The immutable factory
scale table in `src/marbles_factory_scales.h` is automatically compared with
the pinned upstream settings data. No upstream algorithm file is edited here.

The plugin factory GUID is `NT_MULTICHAR('T', 'h', 'M', 'B')`; `Th` reserves
the owner's thorinside namespace.

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
`patches/eurorack/*.patch` files. The initial pinned source compiles without a
patch, so the overlay is intentionally empty. These commands are safe to use
when the upstream checkout is clean:

```sh
./scripts/apply-eurorack-patches.sh
./scripts/remove-eurorack-patches.sh
# or both plus a clean-check:
make verify-overlay
```

They use `git apply` and reverse `git apply`; neither command commits to or
changes the pinned submodule after removal. Add a patch only for a documented
NT compatibility issue and record the behavioral departure and its verification
in `docs/departures.md`.

## Build automation

`.github/workflows/build.yml` is the Linux CI path. It checks out recursive
submodules, builds and inspects the ARM object, and uploads the installable
MicroSD-layout archive. Tag-triggered releases are intentionally left to a
future approved release slice.
