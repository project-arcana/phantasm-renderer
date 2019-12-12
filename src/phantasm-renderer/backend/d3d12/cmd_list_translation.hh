#pragma once

#include <clean-core/array.hh>

#include <phantasm-renderer/backend/command_stream.hh>

#include <phantasm-renderer/backend/d3d12/common/d3d12_fwd.hh>
#include <phantasm-renderer/backend/d3d12/pools/linear_descriptor_allocator.hh>

namespace pr::backend::detail
{
template <class StateT>
struct generic_incomplete_state_cache;
using incomplete_state_cache = generic_incomplete_state_cache<resource_state>;
}

namespace pr::backend::d3d12
{
class ShaderViewPool;
class ResourcePool;
class PipelineStateObjectPool;
class CPUDescriptorLinearAllocator;

struct translator_thread_local_memory
{
    void initialize(ID3D12Device& device);

    CPUDescriptorLinearAllocator lin_alloc_rtvs;
    CPUDescriptorLinearAllocator lin_alloc_dsvs;
};

struct translator_global_memory
{
    void initialize(ID3D12Device* device, ShaderViewPool* sv_pool, ResourcePool* resource_pool, PipelineStateObjectPool* pso_pool)
    {
        this->device = device;
        this->pool_shader_views = sv_pool;
        this->pool_resources = resource_pool;
        this->pool_pipeline_states = pso_pool;
    }

    ID3D12Device* device;
    ShaderViewPool* pool_shader_views;
    ResourcePool* pool_resources;
    PipelineStateObjectPool* pool_pipeline_states;
};

/// responsible for filling command lists, 1 per thread
struct command_list_translator
{
    void initialize(ID3D12Device* device, ShaderViewPool* sv_pool, ResourcePool* resource_pool, PipelineStateObjectPool* pso_pool);

    void translateCommandList(ID3D12GraphicsCommandList* list, pr::backend::detail::incomplete_state_cache* state_cache, std::byte* buffer, size_t buffer_size);

    void execute(cmd::begin_render_pass const& begin_rp);

    void execute(cmd::draw const& draw);

    void execute(cmd::dispatch const& dispatch);

    void execute(cmd::end_render_pass const& end_rp);

    void execute(cmd::transition_resources const& transition_res);

    void execute(cmd::copy_buffer const& copy_buf);

    void execute(cmd::copy_buffer_to_texture const& copy_text);

private:
    // non-owning constant (global)
    translator_global_memory _globals;

    // owning constant (thread local)
    translator_thread_local_memory _thread_local;

    // non-owning dynamic
    pr::backend::detail::incomplete_state_cache* _state_cache = nullptr;
    ID3D12GraphicsCommandList* _cmd_list = nullptr;

    // dynamic state
    struct
    {
        handle::pipeline_state pipeline_state;
        handle::resource index_buffer;
        handle::resource vertex_buffer;

        ID3D12RootSignature* raw_root_sig;
        cc::array<handle::shader_view, limits::max_shader_arguments> shader_views;

        void reset()
        {
            pipeline_state = handle::null_pipeline_state;
            index_buffer = handle::null_resource;
            vertex_buffer = handle::null_resource;

            set_root_sig(nullptr);
        }

        void set_root_sig(ID3D12RootSignature* raw)
        {
            for (auto& sv : shader_views)
                sv = handle::null_shader_view;

            raw_root_sig = raw;
        }

    } _bound;
};

}
