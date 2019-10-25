#pragma once
#ifdef PR_BACKEND_VULKAN

#include <cstring>     // memset
#include <type_traits> // is_pointer

#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

namespace pr::backend::vk
{
/// Zero out a Vulkan struct, while setting the first .sType member correctly
template <class T>
void zero_info_struct(T& val, VkStructureType type)
{
    static_assert(!std::is_pointer_v<T>, "Argument must not be a pointer");
    static_assert(offsetof(T, sType) == 0, "Vulkan info struct must have sType as its first member");
    val.sType = type;
    std::memset(reinterpret_cast<char*>(&val) + sizeof(VkStructureType), 0, sizeof(T) - sizeof(VkStructureType));
}

template <class T>
void zero_struct(T& val)
{
    static_assert(!std::is_pointer_v<T>, "Argument must not be a pointer");
    std::memset(reinterpret_cast<char*>(&val), 0, sizeof(T));
}

}

#endif
