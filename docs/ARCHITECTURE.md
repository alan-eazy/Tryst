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

### 1.2 Shared engine assets

Heavy engine data derives from `EngineAsset` and is published to voices as immutable `shared_ptr<const EngineAsset>` objects outside the render path. The generic engine/voice API can broadcast one asset to every voice without teaching the voice manager about wavetable or sampler internals. This is the base for future sample, granular and spectral asset sharing.

### 1.3 Wavetable assets

`WavetableData` is an immutable frame asset created outside the audio thread. `WavetableEngine` performs linear interpolation both within a frame and between adjacent frames, so wavetable position can be modulated continuously without rebuilding data. The current built-in table contains sine, triangle, saw and square frames. Arbitrary frame sets can already be supplied in memory; file import, analysis and band-limited mipmap generation are intentionally deferred to the asset-loading layer.

### 1.4 Sampler assets and playback

`SampleData` stores immutable mono or stereo sample frames plus source sample rate and root MIDI note. `SamplerEngine` compensates source/host sample-rate differences, pitch-tracks notes relative to the root note, uses linear interpolation, and exposes normalized start/end, loop start/end, loop enable and reverse parameters through the same destination registry as the other engines. The current core is intentionally single-sample-per-slot; multisample key/velocity mapping and file decoding remain asset-layer work rather than audio-thread responsibilities.

### 1.5 Granular playback

`GranularEngine` consumes the same immutable `SampleData` assets as the sampler. Each engine instance owns a fixed pool of 32 grains, so scheduling and rendering require no heap allocation. Grain position, size, density, spray and pitch are exposed as stable normalized destinations, while note pitch and common generator tune are combined with the sample root note and source sample rate. Grains use linear source interpolation and a Hann window; deterministic voice-local pseudo-random spray avoids shared RNG state on the audio thread. The current implementation is a bounded real-time cloud engine rather than an offline time-stretcher; stereo spread, freeze modes and richer grain-window choices remain future extensions.

## 2. Modulation

The modulation matrix is central infrastructure. Engines and effects expose destinations through stable destination IDs; LFOs, envelopes, MSEGs, random sources, macros and expressive controls provide sources.

### 2.1 Normalized destinations

Engine parameters are addressed through stable 32-bit destination IDs derived from persistent string IDs using FNV-1a. The registry checks built-in destination collisions at compile time. Common generator controls such as level and tune share destinations across engines, while engine-specific destinations such as analog shape and wavetable position remain explicit. This gives the modulation layer one normalized 0..1 control surface while preserving long-term preset IDs.

### 2.2 Runtime routing and performance sources

Modulation routes now target a domain, slot and stable parameter destination rather than a bare parameter ID. This lets the same parameter ID be addressed independently on generator slots 1–4 while filters remain voice-local destinations. `ModulationRuntime` stores source values in fixed-size arrays and evaluates routes without heap allocation. Eight macro sources plus X/Y performance sources are wired through `VoiceManager`; changing a performance source refreshes all tracked base targets and broadcasts the resulting normalized values to every voice.

The current performance-source path is intentionally control-rate. It is suitable for macros, XY gestures and host automation, but it is not presented as the final audio-rate LFO/envelope path. Selected audio-rate-capable destinations will later receive a dedicated per-sample or small-block modulation path.

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
