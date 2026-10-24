#include "GranularEngine.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace tryst::dsp
{
GranularEngine::GranularEngine()
    : sample(SampleData::createBasic())
{
}

void GranularEngine::prepare(double newSampleRate, std::uint32_t)
{
    hostSampleRate = newSampleRate > 1.0 ? newSampleRate : 48000.0;
    reset();
}

void GranularEngine::reset() noexcept
{
    clearGrains();
    spawnCountdown = 0.0;
    velocity = 0.0f;
    active = false;
}

void GranularEngine::noteOn(const NoteEvent& note) noexcept
{
    baseMidiNote = static_cast<float>(note.note) + note.tuningOffsetSemitones;
    playedMidiNote = baseMidiNote + note.pitchBendSemitones;
    velocity = std::clamp(note.velocity, 0.0f, 1.0f);
    randomState ^= static_cast<std::uint32_t>(note.note + 1) * 0x9e3779b9u;
    clearGrains();
    spawnCountdown = 0.0;
    active = sample && sample->frames() > 1;
}

void GranularEngine::noteOff(std::int32_t, float) noexcept
{
    // Voice-level amplitude envelopes own release timing. Existing grains and
    // newly scheduled grains continue until the voice envelope finishes.
}

bool GranularEngine::setPitchBendSemitones(float semitones) noexcept
{
    const auto previous = playedMidiNote;
    playedMidiNote = baseMidiNote + std::clamp(semitones, -48.0f, 48.0f);
    const auto ratio = std::pow(2.0, static_cast<double>(playedMidiNote - previous) / 12.0);
    for (auto& grain : grains)
        if (grain.active)
            grain.increment *= ratio;
    return true;
}

void GranularEngine::render(ProcessContext& context) noexcept
{
    if (!active || !sample || sample->frames() < 2 || context.left.empty() || context.right.empty())
        return;

    const auto samples = std::min(context.left.size(), context.right.size());
    const auto normalization = overlapNormalization();

    for (std::size_t frame = 0; frame < samples; ++frame)
    {
        if (spawnCountdown <= 0.0)
        {
            spawnGrain();
            spawnCountdown += spawnIntervalSamples();
        }
        spawnCountdown -= 1.0;

        float left = 0.0f;
        float right = 0.0f;

        for (auto& grain : grains)
        {
            if (!grain.active)
                continue;

            if (grain.position < 0.0 || grain.position > static_cast<double>(sample->frames() - 1))
            {
                grain.active = false;
                continue;
            }

            const auto phase = grain.duration > 1
                ? static_cast<double>(grain.age) / static_cast<double>(grain.duration - 1)
                : 1.0;
            const auto window = static_cast<float>(0.5 - 0.5 * std::cos(2.0 * std::numbers::pi * phase));
            left += interpolate(false, grain.position) * window;
            right += interpolate(true, grain.position) * window;

            grain.position += grain.increment;
            ++grain.age;
            if (grain.age >= grain.duration)
                grain.active = false;
        }

        const auto gain = velocity * level * normalization;
        context.left[frame] += left * gain;
        context.right[frame] += right * gain;
    }
}

bool GranularEngine::setParameter(tryst::params::DestinationId destination, float normalized) noexcept
{
    normalized = std::clamp(normalized, 0.0f, 1.0f);

    if (destination == tryst::params::destination::generatorLevel)
    {
        level = normalized;
        return true;
    }
    if (destination == tryst::params::destination::generatorTune)
    {
        detuneSemitones = (normalized - 0.5f) * 48.0f;
        return true;
    }
    if (destination == tryst::params::destination::granularPosition)
    {
        positionNormalized = normalized;
        return true;
    }
    if (destination == tryst::params::destination::granularSize)
    {
        sizeNormalized = normalized;
        return true;
    }
    if (destination == tryst::params::destination::granularDensity)
    {
        densityNormalized = normalized;
        return true;
    }
    if (destination == tryst::params::destination::granularSpray)
    {
        sprayNormalized = normalized;
        return true;
    }
    if (destination == tryst::params::destination::granularPitch)
    {
        grainPitchSemitones = (normalized - 0.5f) * 48.0f;
        return true;
    }

    return false;
}

bool GranularEngine::setAsset(std::shared_ptr<const EngineAsset> next) noexcept
{
    const auto nextSample = std::dynamic_pointer_cast<const SampleData>(std::move(next));
    if (!nextSample)
        return false;

    setSample(nextSample);
    return true;
}

void GranularEngine::setSample(std::shared_ptr<const SampleData> next) noexcept
{
    if (!next || next->frames() < 2)
        return;

    sample = std::move(next);
    reset();
}

float GranularEngine::interpolate(bool rightChannel, double position) const noexcept
{
    if (!sample || sample->frames() < 2)
        return 0.0f;

    const auto maxIndex = sample->frames() - 1;
    const auto clamped = std::clamp(position, 0.0, static_cast<double>(maxIndex));
    const auto index0 = static_cast<std::size_t>(std::floor(clamped));
    const auto index1 = std::min(index0 + 1, maxIndex);
    const auto blend = static_cast<float>(clamped - static_cast<double>(index0));
    const auto a = rightChannel ? sample->sampleRight(index0) : sample->sampleLeft(index0);
    const auto b = rightChannel ? sample->sampleRight(index1) : sample->sampleLeft(index1);
    return a + (b - a) * blend;
}

float GranularEngine::nextRandomBipolar() noexcept
{
    auto x = randomState;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    randomState = x == 0 ? 0x6d2b79f5u : x;
    const auto unit = static_cast<float>(randomState) / static_cast<float>(0xffffffffu);
    return unit * 2.0f - 1.0f;
}

double GranularEngine::grainIncrement() const noexcept
{
    if (!sample || hostSampleRate <= 1.0)
        return 1.0;

    const auto semitones = playedMidiNote + detuneSemitones + grainPitchSemitones - sample->rootNote();
    const auto pitchRatio = std::pow(2.0, static_cast<double>(semitones) / 12.0);
    return (sample->sourceSampleRate() / hostSampleRate) * pitchRatio;
}

double GranularEngine::grainSizeSeconds() const noexcept
{
    // 10 ms .. 500 ms, with more useful resolution in the short-grain range.
    const auto shaped = static_cast<double>(sizeNormalized) * static_cast<double>(sizeNormalized);
    return 0.010 + shaped * 0.490;
}

double GranularEngine::grainsPerSecond() const noexcept
{
    const auto shaped = static_cast<double>(densityNormalized) * static_cast<double>(densityNormalized);
    return 1.0 + shaped * 79.0;
}

double GranularEngine::spawnIntervalSamples() const noexcept
{
    return std::max(1.0, hostSampleRate / grainsPerSecond());
}

float GranularEngine::overlapNormalization() const noexcept
{
    const auto expectedOverlap = std::max(1.0, grainsPerSecond() * grainSizeSeconds());
    return static_cast<float>(1.0 / std::sqrt(expectedOverlap));
}

void GranularEngine::spawnGrain() noexcept
{
    if (!sample || sample->frames() < 2)
        return;

    auto* grain = static_cast<Grain*>(nullptr);
    for (auto& candidate : grains)
    {
        if (!candidate.active)
        {
            grain = &candidate;
            break;
        }
    }

    if (grain == nullptr)
        return;

    const auto maxFrame = static_cast<double>(sample->frames() - 1);
    const auto basePosition = static_cast<double>(positionNormalized) * maxFrame;
    const auto sprayFrames = static_cast<double>(sprayNormalized) * 0.5 * maxFrame;
    const auto randomOffset = static_cast<double>(nextRandomBipolar()) * sprayFrames;
    const auto duration = static_cast<std::uint32_t>(std::clamp(
        std::llround(grainSizeSeconds() * hostSampleRate), 2LL, 1'000'000LL));

    grain->position = std::clamp(basePosition + randomOffset, 0.0, maxFrame);
    grain->increment = grainIncrement();
    grain->age = 0;
    grain->duration = duration;
    grain->active = true;
}

void GranularEngine::clearGrains() noexcept
{
    for (auto& grain : grains)
        grain = {};
}
