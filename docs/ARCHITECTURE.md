# Tryst Architecture

## 1. Generator slots

Tryst uses generic generator slots instead of hard-wired OSC 1 / OSC 2 sections. Each slot owns an `Engine` implementation. Planned engines:

- Analog / virtual analog
- Wavetable
- Sampler / multisampler
- Granular
- Spectral / resynthesis
- Additive
- Physical modelling
- Noise / texture

The engine API is intentionally host-agnostic. Plugin wrapper code should translate host MIDI/MPE and automation into engine-friendly events rather than coupling DSP code to a particular plugin SDK.

## 2. Modulation

The modulation matrix is central infrastructure. Engines and effects expose destinations through stable destination IDs; LFOs, envelopes, MSEGs, random sources, macros and expressive controls provide sources.

Long-term requirements:

- Bipolar/unipolar modulation
- Voice-local and global routes
- Modulation-of-modulation
- Audio-rate paths for selected destinations
- Curves, slew and quantization
- Visual modulation ranges
- Per-note modulation for MPE / CLAP hosts where available

## 3. Routing graph

The target routing model is a constrained real-time graph:

`Generators -> Mix/Route -> Filters -> FX lanes -> Output`

Parallel and serial paths should be possible without requiring a fully general graph editor for normal use. The graph must be compiled outside the audio thread into a lock-free render plan.

## 4. Performance layer

The performance surface should expose:

- 8 macros
- XY morph control
- Arpeggiator / sequencer entry point
- Master expression controls
- Preset browser and tags

A deeper Design view can expose engines, modulation, routing and effect details.

## 5. Presets

Patch files should be versioned data, not binary object dumps. Stable parameter IDs are a compatibility contract. Migrations should be explicit when schemas change.

Planned patch states:

- Full patch
- Engine component preset
- Filter component preset
- Modulator preset
- FX chain preset
- Four-corner morph snapshots

## 6. Real-time rules

The audio thread must avoid:

- Heap allocation/deallocation
- Mutexes and blocking synchronization
- Filesystem/network access
- UI calls
- Unbounded loops

Expensive asset analysis (sample slicing, spectral analysis, wavetable extraction) belongs on worker threads and publishes immutable data to DSP voices.
