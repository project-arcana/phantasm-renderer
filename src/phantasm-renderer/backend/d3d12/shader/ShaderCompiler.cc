#include "ShaderCompiler.hh"
#ifdef PR_BACKEND_D3D12

#include <cstring>
#include <fstream>
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
    auto const res = ::D3DCompile(shader_code, shader_code_length, nullptr, nullptr, nullptr, entrypoint, target, D3DCOMPILE_OPTIMIZATION_LEVEL3, 0,
                                  &compile_result, nullptr);

    if (res != S_OK || !compile_result)
        return false;

    out_bytecode = D3D12_SHADER_BYTECODE{compile_result->GetBufferPointer(), compile_result->GetBufferSize()};
    return true;
}


bool pr::backend::d3d12::compile_shader_from_file(const char* filename, const char* entrypoint, const char* target, D3D12_SHADER_BYTECODE& out_bytecode)
{
    std::ifstream file(filename);
    if (!file.good())
        return false;

    auto const file_contents = read_entire_stream(file);
    return compile_shader_from_string(file_contents.c_str(), entrypoint, target, out_bytecode);
}

#endif
