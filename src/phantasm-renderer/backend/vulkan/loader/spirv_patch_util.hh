#pragma once

#include <cstddef>

namespace pr::backend::vk::util
{
struct patched_spirv
{
    std::byte* bytecode;
    size_t bytecode_size;
};

[[nodiscard]] patched_spirv create_patched_spirv(std::byte* bytecode, size_t bytecode_size);

/// purely convenience, not to be used in final backend
[[nodiscard]] patched_spirv create_patched_spirv_from_binary_file(char const* filename);

void free_patched_spirv(patched_spirv const& val);

}
