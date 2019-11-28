#include "spirv_patch_util.hh"

#include <clean-core/array.hh>
#include <clean-core/bit_cast.hh>

#include <phantasm-renderer/backend/detail/unique_buffer.hh>
#include <phantasm-renderer/backend/lib/spirv_reflect.hh>
#include <phantasm-renderer/backend/limits.hh>

namespace
{
void patchSpvReflectShader(SpvReflectShaderModule& module)
{
    uint32_t num_bindings;
    spvReflectEnumerateDescriptorBindings(&module, &num_bindings, nullptr);
    cc::array<SpvReflectDescriptorBinding*> bindings(num_bindings);
    spvReflectEnumerateDescriptorBindings(&module, &num_bindings, bindings.data());

    for (auto b : bindings)
    {
        if (b->resource_type == SPV_REFLECT_RESOURCE_FLAG_CBV)
        {
            auto const new_set = b->set + pr::backend::limits::max_shader_arguments;
            spvReflectChangeDescriptorBindingNumbers(&module, b, b->binding, new_set);
        }
    }
}

}

pr::backend::vk::util::patched_spirv pr::backend::vk::util::create_patched_spirv(std::byte* bytecode, size_t bytecode_size)
{
    patched_spirv res;

    SpvReflectShaderModule module;
    spvReflectCreateShaderModule(bytecode_size, bytecode, &module);

    patchSpvReflectShader(module);

    res.bytecode_size = spvReflectGetCodeSize(&module);
    res.bytecode = cc::bit_cast<std::byte*>(module._internal->spirv_code);

    // spirv-reflect internally checks if this field is a nullptr before calling ::free,
    // so we can keep it alive by setting this
    module._internal->spirv_code = nullptr;

    spvReflectDestroyShaderModule(&module);
    return res;
}


void pr::backend::vk::util::free_patched_spirv(const pr::backend::vk::util::patched_spirv& val)
{
    // do the same thing spirv-reflect would have done in spvReflectDestroyShaderModule
    ::free(val.bytecode);
}

pr::backend::vk::util::patched_spirv pr::backend::vk::util::create_patched_spirv_from_binary_file(const char* filename)
{
    auto const binary_data = detail::unique_buffer::create_from_binary_file(filename);
    CC_RUNTIME_ASSERT(binary_data.is_valid() && "Couldnt open SPIR-V binary");
    return create_patched_spirv(binary_data.get(), binary_data.size());
}
