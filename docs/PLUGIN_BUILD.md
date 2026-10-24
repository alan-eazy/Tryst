# Tryst plugin builds

The DSP/core build remains SDK-free by default. Enable the plugin formats explicitly:

```sh
cmake -S . -B build -DTRYST_BUILD_PLUGINS=ON -DTRYST_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

`TRYST_BUILD_PLUGINS=ON` pins `free-audio/clap-wrapper` to `v0.16.0`. The wrapper is allowed to download its CLAP/VST3 dependencies at configure time and emits both CLAP and VST3 from the same CLAP-first implementation.

## Current host surface

- instrument with one stereo audio output and no audio input
- one native CLAP note input
- sample-accurate note, pressure, brightness/timbre, tuning and parameter events
- stable normalized host parameters backed by Tryst persistent parameter IDs
- CLAP state save/load using the existing versioned Tryst patch format
- VST3 generation through `clap-wrapper`
- fixed-capacity event and note tracking in the audio callback

## Real-time boundary

Audio processing performs no plugin-topology rebuilds and does not grow event containers. Host parameter values use atomic preallocated storage so state reads do not race sample-accurate automation. Patch loads which arrive while the plugin is active are held as pending state and request a host restart; they are applied only after processing is stopped/deactivated.

## Deliberate first-wrapper limits

The first CLAP adapter accepts the native CLAP note dialect rather than advertising raw MIDI/MIDI2 input. Note choke currently follows Tryst's normal release envelope because the core does not yet expose an immediate voice-kill event. GUI/editor support is intentionally separate from this wrapper milestone.
