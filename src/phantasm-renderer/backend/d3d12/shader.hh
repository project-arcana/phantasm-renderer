#pragma once

#include <phantasm-renderer/backend/d3d12/common/shared_com_ptr.hh>
#include <phantasm-renderer/backend/d3d12/common/d3d12_fwd.hh>


namespace pr::backend::d3d12
{
enum class shader_domain
{
    pixel,
    vertex,
    domain,
    hull,
    geometry,
    compute
};

struct shader
{
    shader_domain domain;
    shared_com_ptr<ID3DBlob> blob;

    bool is_valid() const { return blob.is_valid(); }
};

[[nodiscard]] shader compile_shader_from_string(char const* shader_code, shader_domain domain, char const* entrypoint = nullptr);
[[nodiscard]] shader compile_shader_from_file(char const* filename, shader_domain domain, char const* entrypoint = nullptr);

}

