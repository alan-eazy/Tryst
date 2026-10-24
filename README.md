# Tryst

**Tryst** is a modular, performance-focused software synthesizer intended to grow into a true Swiss-army instrument: multiple synthesis engines, deep modulation, flexible routing, expressive performance controls, and a streamlined sound-design workflow.

## Design principles

1. **Any source, anywhere** — generator slots host interchangeable synthesis engines.
2. **Modulate almost anything** — modulation is a first-class system, not a feature bolted onto each engine.
3. **Fast to play, deep to design** — a simple performance surface sits above a more powerful design view.
4. **Safe real-time DSP** — no allocations, locks, file I/O, or blocking work on the audio thread.
5. **Stable preset model** — parameter IDs and patch serialization are designed for long-term compatibility.

## Initial architecture

- Four generic generator slots
- Engine interface for analog, wavetable, sampler, granular, spectral, additive, physical-model and noise engines
- Working morphable wavetable engine with immutable table assets and frame interpolation
- Working sampler engine with immutable mono/stereo assets, pitch tracking, regions, looping and reverse playback
- Dual per-voice multimode filters with serial/parallel routing
- Central modulation matrix with slot-aware targets and bipolar depth
- Working 8-macro + XY modulation runtime for generator and filter destinations
- Voice-local and global modulation distinction
- Stable parameter registry with normalized runtime destination IDs
- Roadmap for routing, filters, effects, MPE, microtuning and preset morphing

See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) and [`docs/ROADMAP.md`](docs/ROADMAP.md).

## Status

The DSP core now contains the architecture foundation, analog, wavetable and sampler engines, 16-voice polyphony with deterministic oldest-voice stealing, a per-voice ADSR amplitude envelope, a reusable LFO, stable normalized parameter destinations, dual multimode filters, live macro/XY modulation routing, and smoke tests. The wavetable engine currently supports immutable in-memory frame sets plus continuous frame morphing; file import and band-limited mipmaps are still pending. The sampler supports in-memory mono/stereo assets and loop regions; multisample mapping and file decoding are still pending. Macro/XY routing currently updates at control rate; dedicated audio-rate modulation paths remain planned. Plugin-wrapper integration remains deliberately separate so the DSP core stays testable and host-agnostic.
