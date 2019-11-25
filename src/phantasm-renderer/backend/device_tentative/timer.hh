#pragma once

#include <chrono>

namespace pr::backend::device
{
class Timer
{
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> mStart;

public:
    Timer() { restart(); }

    /// Restart the timer
    void restart() { mStart = std::chrono::high_resolution_clock::now(); }

    /// Get the duration since last restart
    float elapsedSeconds() const { return std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - mStart).count(); }
    double elapsedSecondsD() const { return std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - mStart).count(); }

    /// Get the duration since last restart in milliseconds
    float elapsedMilliseconds() const { return std::chrono::duration<float, std::milli>(std::chrono::high_resolution_clock::now() - mStart).count(); }
    double elapsedMillisecondsD() const
    {
        return std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - mStart).count();
    }

    /// Get the duration since last restart (integer, nanoseconds)
    int64_t elapsedNanoseconds() const { return std::chrono::nanoseconds{std::chrono::high_resolution_clock::now() - mStart}.count(); }
};
}
