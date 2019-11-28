#pragma once

#include <cstddef>

namespace pr::backend::vk::util
{
struct patched_spirv
{
    std::byte* bytecode;
    size_t bytecode_size;
};

/// we have to shift all CBVs up by [max num shader args] sets to make our API work in vulkan
/// unlike the register-to-binding shift with -fvk-[x]-shift, this cannot be done with DXC flags
/// instead we provide these helpers which use the spriv-reflect library to do the same
[[nodiscard]] patched_spirv create_patched_spirv(std::byte* bytecode, size_t bytecode_size);

/// purely convenience, not to be used in final backend
[[nodiscard]] patched_spirv create_patched_spirv_from_binary_file(char const* filename);

void free_patched_spirv(patched_spirv const& val);

}
