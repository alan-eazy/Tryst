# Tryst Patch Format

Tryst patches are versioned UTF-8 text data. They are not binary dumps of DSP objects, so patches can be inspected, migrated and tested independently of plugin wrappers.

## Schema 4

Every patch starts with:

```text
TRYST_PATCH 4
```

and ends with `end`. Supported records include:

```text
engine <slot> <enabled> <engine-kind>
effect <slot> <enabled> <effect-kind>
fx_order <slot0> <slot1> <slot2> <slot3>
amp <attack-seconds> <decay-seconds> <sustain-0..1> <release-seconds>
macro <index> <0..1>
morph <x-0..1> <y-0..1>
lfo <index> <rate-hz> <waveform>
mseg <index> <rate-hz> <loop> <point-count> <phase0> <value0> ...
step <index> <rate-hz> <loop> <step-count> <value0> ...
tuning <midi-note-0..127> <semitone-offset>
param <domain> <slot> <stable-parameter-id> <0..1>
route <control|fast> <source-type> <source-index> <domain> <slot> <stable-parameter-id> <depth> <curve> <enabled>
```

Domains now include `generator`, `filter`, `effect` and `global`. Effect kinds currently include `drive` and `delay`. `fx_order` is a permutation of the four stable FX slot IDs: automation remains attached to a slot while the order record controls signal-flow position.

The codec writes persistent parameter names such as `engine.wavetable.position` and `fx.delay.feedback`, not runtime hash values. Runtime destination hashes are an implementation detail; string IDs are the compatibility contract.

## Assets

Schema 4 deliberately does not embed sample, wavetable or spectral asset bytes. Engine assets remain immutable external data and need a separate asset-reference/resolution layer. Delay memory is runtime state and is also never serialized.

## Loading and migration

`PatchCodec` validates schema versions, ranges, enum names, registered parameter IDs and FX-order permutations before returning a `PatchState`. `PatchApplier` rebuilds topology and control state through `VoiceManager`.

Schemas 1, 2 and 3 remain readable. Older patches migrate with empty/default newer systems: schema 1 gains MSEG/step defaults, schema 2 gains equal-temperament tuning, and schema 3 gains an empty FX rack in identity order. Re-encoding writes schema 4. Unsupported future schema versions fail explicitly.
