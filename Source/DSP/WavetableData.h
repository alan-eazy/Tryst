#pragma once

#include <cstddef>
#include <memory>
#include <vector>

namespace tryst::dsp
{
class WavetableData
{
public:
    [[nodiscard]] static std::shared_ptr<const WavetableData> createBasic(std::size_t tableSize = 2048);
    [[nodiscard]] static std::shared_ptr<const WavetableData> fromFrames(std::vector<float> samples,
                                                                        std::size_t frameCount,
                                                                        std::size_t tableSize);

    [[nodiscard]] std::size_t frames() const noexcept { return frameCount; }
    [[nodiscard]] std::size_t samplesPerFrame() const noexcept { return tableSize; }
    [[nodiscard]] float sample(std::size_t frame, std::size_t index) const noexcept;

private:
    WavetableData(std::vector<float> data, std::size_t frames, std::size_t size) noexcept;

    std::vector<float> data;
    std::size_t frameCount = 0;
    std::size_t tableSize = 0;
};
}
