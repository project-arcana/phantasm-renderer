#pragma once

#include <phantasm-renderer/backend/types.hh>

#include <phantasm-renderer/backend/d3d12/common/d3d12_sanitized.hh>
#include <phantasm-renderer/backend/d3d12/common/shared_com_ptr.hh>
#include <phantasm-renderer/backend/detail/unique_buffer.hh>


namespace pr::backend::d3d12
{
struct shader
{
    shader_domain domain;
    backend::detail::unique_buffer bytecode;

    bool is_valid() const { return bytecode.is_valid(); }
    D3D12_SHADER_BYTECODE get_bytecode() const { return {bytecode.get(), bytecode.size()}; }
};

[[nodiscard]] shader load_binary_shader_from_file(char const* filename, shader_domain domain);

}
