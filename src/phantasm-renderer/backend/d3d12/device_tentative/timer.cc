#include "timer.hh"

#include <clean-core/native/win32_sanitized.hh>

pr::backend::d3d12::Timer::Timer()
{
    LARGE_INTEGER frequency, timestamp;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&timestamp);

    m_frequency = frequency.QuadPart;
    m_timestamp = timestamp.QuadPart;
}

void pr::backend::d3d12::Timer::reset()
{
    LARGE_INTEGER timestamp;
    QueryPerformanceCounter(&timestamp);
    m_timestamp = timestamp.QuadPart;
}

double pr::backend::d3d12::Timer::getElapsedTime() const
{
    LARGE_INTEGER currentTime = {};
    QueryPerformanceCounter(&currentTime);
    return static_cast<double>(currentTime.QuadPart - m_timestamp) / static_cast<double>(m_frequency);
}
