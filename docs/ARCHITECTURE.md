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

## 1.1 Polyphonic voice layer

`VoiceManager` owns a fixed pool of 16 voices. Each voice owns four generator slots plus preallocated stereo scratch buffers and a voice-local amplitude envelope. Note allocation prefers inactive voices and deterministically steals the oldest active voice when the pool is exhausted. This keeps the render path bounded and allocation-free.

Engine creation and replacement happen on the control side. The audio render path only uses already-prepared engine instances.

### 1.2 Wavetable assets

`WavetableData` is an immutable frame asset created outside the audio thread. `WavetableEngine` performs linear interpolation both within a frame and between adjacent frames, so wavetable position can be modulated continuously without rebuilding data. The current built-in table contains sine, triangle, saw and square frames. Arbitrary frame sets can already be supplied in memory; file import, analysis and band-limited mipmap generation are intentionally deferred to the asset-loading layer.

## 2. Modulation

The modulation matrix is central infrastructure. Engines and effects expose destinations through stable destination IDs; LFOs, envelopes, MSEGs, random sources, macros and expressive controls provide sources.

### 2.1 Normalized destinations

Engine parameters are addressed through stable 32-bit destination IDs derived from persistent string IDs using FNV-1a. The registry checks built-in destination collisions at compile time. Common generator controls such as level and tune share destinations across engines, while engine-specific destinations such as analog shape and wavetable position remain explicit. This gives the modulation layer one normalized 0..1 control surface while preserving long-term preset IDs.

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

### 3.1 Dual voice filters

Each voice owns two topology-preserving-transform state-variable filters. Each filter can operate as low-pass, band-pass, high-pass or notch, with logarithmic cutoff mapping and resonance control. The two filters can run in series or in parallel; parallel mode exposes a blend control between both filter outputs. Filter state is voice-local, while parameters are broadcast through `VoiceManager` and use the same stable destination registry as generator parameters.

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
