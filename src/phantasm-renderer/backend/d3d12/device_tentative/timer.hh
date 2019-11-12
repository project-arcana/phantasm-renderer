#pragma once

#include <cstdint>

namespace pr::backend::d3d12
{
class Timer
{
    uint64_t m_frequency;
    uint64_t m_timestamp;

public:
    Timer();

    void reset();
    [[nodiscard]] double getElapsedTime() const;
};
}
