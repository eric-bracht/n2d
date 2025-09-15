# Normal Map to Displacement Map Converter

This project provides a CPU-only converter from tangent-space normal maps to scalar displacement maps.

## Building on Windows
1. Install [vcpkg](https://github.com/microsoft/vcpkg) and obtain the commit SHA of your clone:
   `git -C %VCPKG_ROOT% rev-parse HEAD`.
2. Replace the `builtin-baseline` placeholders in `vcpkg.json` and `vcpkg-configuration.json` with that SHA.
3. Configure: `cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake`.
4. Build: `cmake --build build --config Release`.

## Determinism
Pass `--deterministic` to the CLI or set `BakeParams::deterministic` to enforce static
partitioning and strict floating point behavior.

## Testing
Tests prefer the binary assets under `testdata/` when available. Edge cases and golden
scenarios generate tiny temporary fixtures at runtime and clean them up afterwards.
Run tests with `ctest --test-dir build`.

