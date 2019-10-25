#pragma once
#ifdef PR_BACKEND_VULKAN

namespace pr::backend::vk
{
class Device
{ // reference type
public:
    Device(Device const&) = delete;
    Device(Device&&) noexcept = delete;
    Device& operator=(Device const&) = delete;
    Device& operator=(Device&&) noexcept = delete;

    Device() = default;


private:
};
}

#endif
