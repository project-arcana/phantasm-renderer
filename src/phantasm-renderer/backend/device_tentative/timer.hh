#pragma once

namespace pr::backend::device
{
class Timer
{
    long long mResolution;
    long long mStartTime;

public:
    Timer();

    void reset();
    [[nodiscard]] double getElapsedTime() const;
};
}
