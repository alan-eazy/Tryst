# Tryst

**Tryst** is a modular, performance-focused software synthesizer intended to grow into a true Swiss-army instrument: multiple synthesis engines, deep modulation, flexible routing, expressive performance controls, and a streamlined sound-design workflow.

## Design principles

1. **Any source, anywhere** — generator slots host interchangeable synthesis engines.
2. **Modulate almost anything** — modulation is a first-class system, not a feature bolted onto each engine.
3. **Fast to play, deep to design** — a simple performance surface sits above a more powerful design view.
4. **Safe real-time DSP** — no allocations, locks, file I/O, or blocking work on the audio thread.
5. **Stable preset model** — parameter IDs and patch serialization are designed for long-term compatibility.

## Initial architecture

- Four generic generator slots per voice
- Engine interface for analog, wavetable, sampler, granular, spectral, additive, physical-model and noise engines
- 16-voice polyphonic manager with deterministic oldest-voice stealing
- Per-voice ADSR amplitude envelope
- Reusable LFO modulation source
- Central modulation matrix with bipolar depth
- Macro/XY-ready modulation sources
- Voice-local and global modulation distinction
- Parameter registry with stable IDs
- Roadmap for routing, filters, effects, MPE, microtuning and preset morphing

See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) and [`docs/ROADMAP.md`](docs/ROADMAP.md).

## Status

The DSP core now contains the architecture foundation, a reference analog engine, 16-voice polyphony with deterministic oldest-voice stealing, a per-voice ADSR amplitude envelope, a reusable LFO and smoke tests. Plugin-wrapper integration remains deliberately separate so the DSP core stays testable and host-agnostic.
