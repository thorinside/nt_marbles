# Marbles DSP boundary

The implementation core is compiled **directly** from the pinned
`third_party/eurorack` submodule. No Marbles algorithm source is copied into
this repository.

The current foundation compiles the T and X/Y generator translation units:

- `marbles/random/t_generator.cc`
- `marbles/random/x_y_generator.cc`

Future work must put only NT-to-Marbles signal conversion, parameter scaling,
and output representation code in this directory. It must not edit files under
`third_party/eurorack`; compatibility edits belong in `patches/eurorack/` and
are applied with `scripts/apply-eurorack-patches.sh`.
