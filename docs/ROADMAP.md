# Tryst Roadmap

## P0 — Foundation

- [x] Generic engine interface
- [x] Generator slot abstraction
- [x] Stable parameter ID policy
- [x] Initial modulation route model
- [x] Reference analog engine
- [x] Voice manager and polyphony
- [x] ADSR + LFO modulators
- [x] Parameter registry / normalized destination map
- [ ] Host wrapper (VST3 + CLAP first)
- [ ] Basic preset serialization
- [x] Unit tests and DSP smoke tests

## P1 — Core instrument

- [ ] Four generator slots
- [x] Dual multimode filters with serial/parallel routing
- [~] Wavetable engine (core engine complete; file import + band-limited mipmaps pending)
- [ ] Sampler / multisample engine
- [ ] Granular engine
- [ ] Noise / texture generator
- [ ] MSEG and step modulation
- [ ] 8 macros + XY control
- [ ] Flexible FX rack
- [ ] MPE and microtuning
- [ ] Smart randomize / mutate with locks

## P2 — Signature features

- [ ] Audio-to-synthesis lab: audio -> wavetable / granular / sampler / spectral / additive
- [ ] Four-corner full-patch morphing
- [ ] Spectral/resynthesis engine
- [ ] Additive engine
- [ ] Physical-modelling engines
- [ ] Modulation combiners (math, quantize, slew)
- [ ] Envelope follower / external audio modulation
- [ ] Euclidean / probability / ratchet sequencing
- [ ] Component preset browser

## P3 — Polish and scale

- [ ] CPU-aware oversampling strategy
- [ ] SIMD/vectorized hot paths
- [ ] Multicore voice rendering where beneficial
- [ ] Spectroscope / oscilloscope / modulation visualization
- [ ] Preset tagging, search and favorites
- [ ] Accessibility and scalable UI
- [ ] Cross-platform CI and release packaging
