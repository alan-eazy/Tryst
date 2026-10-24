# Tryst Patch Format

Tryst patches are versioned UTF-8 text data. They are not binary dumps of DSP objects, so patches can be inspected, migrated and tested independently of plugin wrappers.

## Schema 2

Every patch starts with:

```text
TRYST_PATCH 2
```

and ends with:

```text
end
```

Supported records are:

```text
engine <slot> <enabled> <engine-kind>
amp <attack-seconds> <decay-seconds> <sustain-0..1> <release-seconds>
macro <index> <0..1>
morph <x-0..1> <y-0..1>
lfo <index> <rate-hz> <waveform>
mseg <index> <rate-hz> <loop> <point-count> <phase0> <value0> ...
step <index> <rate-hz> <loop> <step-count> <value0> ...
param <domain> <slot> <stable-parameter-id> <0..1>
route <control|fast> <source-type> <source-index> <domain> <slot> <stable-parameter-id> <depth> <curve> <enabled>
```

The codec writes parameter names such as `engine.wavetable.position` rather than their runtime hash values. Runtime destination hashes are an implementation detail; the persistent string IDs are the compatibility contract.

## Assets

Schema 2 deliberately does not embed sample, wavetable or spectral asset bytes. Engine assets are immutable external data and need a separate asset-reference/resolution layer. A patch can therefore restore synthesis topology and parameters without accidentally turning preset state into a large binary audio container.

## Loading

`PatchCodec` validates the schema, ranges, MSEG point order, enum names and registered parameter IDs before returning a `PatchState`. `PatchApplier` then rebuilds engine selection, parameters, envelope settings, LFO/MSEG/step configuration, modulation routes, macros and XY values through the public `VoiceManager` control API.

Unsupported future schema versions fail explicitly. Future migrations should convert older `PatchState` schemas before applying them rather than adding hidden compatibility branches inside DSP classes.

## Migration

Schema 1 patches remain readable. The decoder migrates them in memory to schema 2, leaving the new MSEG and step configurations empty/default. Re-encoding the migrated state writes a schema 2 patch. Future migrations should follow the same explicit pattern rather than branching compatibility behavior inside DSP classes.
