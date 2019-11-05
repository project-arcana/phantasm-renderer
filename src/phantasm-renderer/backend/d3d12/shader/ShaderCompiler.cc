#include "ShaderCompiler.hh"

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

}

bool pr::backend::d3d12::compile_shader_from_string(const char* shader_code, const char* entrypoint, const char* target, D3D12_SHADER_BYTECODE& out_bytecode)
{
    if (!shader_code || !entrypoint || !target)
        return false;

    auto const shader_code_length = std::strlen(shader_code);

    ID3DBlob* compile_result = nullptr;
    ID3DBlob* error_result = nullptr;
    auto const res = ::D3DCompile(shader_code, shader_code_length, nullptr, nullptr, nullptr, entrypoint, target, D3DCOMPILE_OPTIMIZATION_LEVEL3, 0,
                                  &compile_result, &error_result);

    if (res != S_OK || !compile_result)
    {
        if (error_result)
        {
            std::cerr << "Error compiling shader: \n  " << (static_cast<char const*>(error_result->GetBufferPointer())) << std::endl;
            error_result->Release();
        }

        if (compile_result)
            compile_result->Release();

        return false;
    }

    out_bytecode = D3D12_SHADER_BYTECODE{compile_result->GetBufferPointer(), compile_result->GetBufferSize()};
    return true;
}


bool pr::backend::d3d12::compile_shader_from_file(const char* filename, const char* entrypoint, const char* target, D3D12_SHADER_BYTECODE& out_bytecode)
{
    std::ifstream file(filename);
    if (!file.good())
    {
        std::cerr << "Failed to open shader at " << filename << std::endl;
        return false;
    }

    auto const file_contents = read_entire_stream(file);
    return compile_shader_from_string(file_contents.c_str(), entrypoint, target, out_bytecode);
}

