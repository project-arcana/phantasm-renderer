#pragma once
#ifdef PR_BACKEND_D3D12

struct D3D12_SHADER_BYTECODE;

namespace pr::backend::d3d12
{
[[nodiscard]] bool compile_shader_from_string(char const* shader_code, char const* entrypoint, char const* target, D3D12_SHADER_BYTECODE& out_bytecode);
[[nodiscard]] bool compile_shader_from_file(char const* filename, char const* entrypoint, char const* target, D3D12_SHADER_BYTECODE& out_bytecode);

}

#endif
