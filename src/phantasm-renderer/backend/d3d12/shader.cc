#include "shader.hh"

#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

// NOTE: d3dcompiler is not included in d3d12_sanitized, yet we need it here
// TODO: use new dx12 shader compiler
#include <d3d12.h>
#include <d3dcompiler.h>

//#include <phantasm-renderer/backend/d3d12/common/d3d12_sanitized.hh>
#include <phantasm-renderer/backend/d3d12/common/verify.hh>


namespace
{
[[nodiscard]] std::string read_entire_stream(std::istream& in)
{
    std::string ret;
    char buffer[4096];
    while (in.read(buffer, sizeof(buffer)))
        ret.append(buffer, sizeof(buffer));
    ret.append(buffer, size_t(in.gcount()));
    return ret;
}

[[nodiscard]] constexpr char const* get_domain_literal(pr::backend::d3d12::shader_domain domain)
{
    switch (domain)
    {
    case pr::backend::d3d12::shader_domain::pixel:
        return "ps_5_0";
    case pr::backend::d3d12::shader_domain::vertex:
        return "vs_5_0";
    case pr::backend::d3d12::shader_domain::domain:
        return "ds_5_0";
    case pr::backend::d3d12::shader_domain::hull:
        return "hs_5_0";
    case pr::backend::d3d12::shader_domain::geometry:
        return "gs_5_0";
    case pr::backend::d3d12::shader_domain::compute:
        return "cs_5_0";
    }
}

[[nodiscard]] constexpr char const* get_default_entrypoint(pr::backend::d3d12::shader_domain domain)
{
    switch (domain)
    {
    case pr::backend::d3d12::shader_domain::pixel:
        return "mainPS";
    case pr::backend::d3d12::shader_domain::vertex:
        return "mainVS";
    case pr::backend::d3d12::shader_domain::domain:
        return "mainDS";
    case pr::backend::d3d12::shader_domain::hull:
        return "mainHS";
    case pr::backend::d3d12::shader_domain::geometry:
        return "mainGS";
    case pr::backend::d3d12::shader_domain::compute:
        return "mainCS";
    }
}
}

pr::backend::d3d12::shader pr::backend::d3d12::compile_shader_from_string(char const* shader_code, shader_domain domain, char const* entrypoint)
{
    shader res;
    res.domain = domain;

    if (!shader_code)
        return res;

    auto const shader_code_length = std::strlen(shader_code);

    shared_com_ptr<ID3DBlob> error_result;
    auto const hr = ::D3DCompile(shader_code, shader_code_length, nullptr, nullptr, nullptr, entrypoint ? entrypoint : get_default_entrypoint(domain),
                                 get_domain_literal(domain), D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, res.blob.override(), error_result.override());

    if (hr != S_OK || !res.blob.is_valid())
    {
        if (error_result.is_valid())
        {
            std::cerr << "Error compiling shader: \n  " << (static_cast<char const*>(error_result->GetBufferPointer())) << std::endl;
        }

        // return invalidated result
        res.blob = nullptr;
        return res;
    }

    return res;
}


pr::backend::d3d12::shader pr::backend::d3d12::compile_shader_from_file(char const* filename, shader_domain domain, char const* entrypoint)
{
    std::ifstream file(filename);
    if (!file.good())
    {
        std::cerr << "Failed to open shader at " << filename << std::endl;
        return shader();
    }

    auto const file_contents = read_entire_stream(file);
    return compile_shader_from_string(file_contents.c_str(), domain, entrypoint);
}
