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

### 1.6 Procedural noise and texture

`NoiseEngine` is an asset-free procedural source. It generates independent deterministic white streams per stereo side, derives pink noise with a fixed filter bank and brown noise with a bounded integrator, then continuously morphs white -> pink -> brown through the `engine.noise.color` destination. `engine.noise.stereo` crossfades from mono-correlated output to independent stereo channels. `engine.noise.texture` blends the colored stream toward a sample-and-hold signal; common generator tune changes the hold rate in semitone ratios, turning the same source into pitched digital texture. All state is voice-local and bounded, with no allocation or shared random generator in render.

## 2. Modulation

The modulation matrix is central infrastructure. Engines and effects expose destinations through stable destination IDs; LFOs, envelopes, MSEGs, random sources, macros and expressive controls provide sources.

### 2.1 Normalized destinations

Engine parameters are addressed through stable 32-bit destination IDs derived from persistent string IDs using FNV-1a. The registry checks built-in destination collisions at compile time. Common generator controls such as level and tune share destinations across engines, while engine-specific destinations such as analog shape and wavetable position remain explicit. This gives the modulation layer one normalized 0..1 control surface while preserving long-term preset IDs.

### 2.2 Runtime routing and performance sources

Modulation routes now target a domain, slot and stable parameter destination rather than a bare parameter ID. This lets the same parameter ID be addressed independently on generator slots 1–4 while filters remain voice-local destinations. `ModulationRuntime` stores source values in fixed-size arrays and evaluates routes without heap allocation. Eight macro sources plus X/Y performance sources are wired through `VoiceManager`; changing a performance source refreshes all tracked base targets and broadcasts the resulting normalized values to every voice.

The performance-source path is intentionally control-rate. It is suitable for macros, XY gestures and host automation.

### 2.3 Voice-local fast modulation

Each voice also owns a `FastModulationRuntime` with four independent LFOs. Continuous destinations marked `audioRateCapable` in the parameter registry may be routed through this lane. Voice rendering is split into fixed 16-sample chunks; at each chunk boundary the voice evaluates fast routes on top of the current macro/XY-adjusted base values, applies generator/filter destinations directly, renders the chunk, then advances its LFO phases. LFO phases reset when a voice starts, making the current implementation deterministic and voice-local. Toggle/discrete destinations are rejected from the fast lane. This is a bounded high-rate control path, not a claim of true per-sample modulation; genuinely audio-rate FM/PM and other sensitive destinations can receive specialized per-sample paths later.

Current fast sources are voice-local and note-resettable: four LFOs, two MSEGs (up to 16 linear points each) and two step sequencers (up to 16 bipolar values each). They share the same bounded 16-sample update lane and stable target IDs. MSEGs and steps can loop or run one-shot; their rates are currently expressed as complete cycles per second until host-tempo transport is introduced by the wrapper layer.

Per-note expression also enters this voice-local lane. Velocity, bipolar key tracking, pressure, timbre and pitch bend are exposed through the existing `SourceType` IDs, so MPE-style expression can target any eligible generator or filter destination. Pressure, timbre and pitch bend may be updated while a note is active; pitch-bend changes are also forwarded to pitch-aware engines so analog, wavetable, sampler and granular sources retune immediately.

Long-term requirements:

- Bipolar/unipolar modulation
- Voice-local and global routes
- Modulation-of-modulation
- Audio-rate paths for selected destinations
- Curves, slew and quantization
- Visual modulation ranges
- Per-note modulation for MPE / CLAP hosts where available

## 2.4 Microtuning

`TuningTable` stores a bounded 128-note table of semitone offsets from 12-TET. `VoiceManager` applies the note-specific offset when a voice starts, separately from live pitch bend, so tuning and performance expression remain independent. A 12-value octave map can also be repeated across the MIDI range. Patch schema 3 stores only non-zero per-note offsets. External Scala/KBM and MTS-ESP resolution remain wrapper/asset-layer work.

## 3. Routing graph

The target routing model is a constrained real-time graph:

`Generators -> Mix/Route -> Filters -> FX lanes -> Output`

Parallel and serial paths should be possible without requiring a fully general graph editor for normal use. The graph must be compiled outside the audio thread into a lock-free render plan.

### 3.1 Dual voice filters

Each voice owns two topology-preserving-transform state-variable filters. Each filter can operate as low-pass, band-pass, high-pass or notch, with logarithmic cutoff mapping and resonance control. The two filters can run in series or in parallel; parallel mode exposes a blend control between both filter outputs. Filter state is voice-local, while parameters are broadcast through `VoiceManager` and use the same stable destination registry as generator parameters.

### 3.2 Global FX rack

After all active voices are summed, `VoiceManager` processes one global `FxRack`. The rack has four stable automation slots and a separate four-position processing-order array. Reordering therefore changes signal flow without changing which slot a macro, patch parameter or automation target addresses. Effect construction and delay-buffer allocation happen on the control/prepare side; `process()` performs no heap allocation.

The first modules are `DriveEffect` (normalized soft clipping with dry/wet mix) and `DelayEffect` (preallocated stereo circular buffers, interpolated taps and cross-feedback). FX destinations use the same stable parameter registry and `TargetId` structure as generators/filters. Macro/XY control-rate routes may target an FX slot; the voice-local fast lane deliberately rejects FX targets because global post-voice processing is not voice-local.

## 4. Performance layer

The performance surface should expose:

- 8 macros
- XY morph control
- Arpeggiator / sequencer entry point
- Master expression controls
- Preset browser and tags

A deeper Design view can expose engines, modulation, routing and effect details.

## 5. Presets

Patch files are versioned data, not binary object dumps. Stable parameter string IDs are the compatibility contract; the 32-bit runtime destination hashes are not persisted. `PatchCodec` schema 4 serializes engine topology, normalized parameters, ADSR, macros/XY, voice LFO/MSEG/step settings, per-note microtuning offsets, FX topology/order/parameters, and both control-rate and fast modulation routes into deterministic UTF-8 text. Schemas 1, 2 and 3 are migrated in memory on decode. `PatchApplier` rebuilds that state through the public `VoiceManager` control API. Decode errors and unsupported schema versions fail explicitly rather than being silently coerced.

Heavy engine assets are deliberately not embedded in schema 4. Sample, wavetable and future spectral data remain immutable external assets and will use a separate reference/resolution layer. See [`PATCH_FORMAT.md`](PATCH_FORMAT.md).

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
