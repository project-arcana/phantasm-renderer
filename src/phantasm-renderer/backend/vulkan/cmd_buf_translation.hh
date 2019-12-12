#pragma once

#include <clean-core/array.hh>

#include <phantasm-renderer/backend/command_stream.hh>

#include <phantasm-renderer/backend/vulkan/common/vk_incomplete_state_cache.hh>
#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

namespace pr::backend::vk
{
class ShaderViewPool;
class ResourcePool;
class PipelinePool;
class CommandListPool;

struct translator_thread_local_memory
{
    void initialize(VkDevice)
    { /* TODO */
    }
};

struct translator_global_memory
{
    void initialize(VkDevice device, ShaderViewPool* sv_pool, ResourcePool* resource_pool, PipelinePool* pso_pool, CommandListPool* cmd_pool)
    {
        this->device = device;
        this->pool_shader_views = sv_pool;
        this->pool_resources = resource_pool;
        this->pool_pipeline_states = pso_pool;
        this->pool_cmd_lists = cmd_pool;
    }

    VkDevice device;
    ShaderViewPool* pool_shader_views;
    ResourcePool* pool_resources;
    PipelinePool* pool_pipeline_states;
    CommandListPool* pool_cmd_lists;
};

/// responsible for filling command lists, 1 per thread
struct command_list_translator
{
    void initialize(VkDevice device, ShaderViewPool* sv_pool, ResourcePool* resource_pool, PipelinePool* pso_pool, CommandListPool* cmd_pool)
    {
        _globals.initialize(device, sv_pool, resource_pool, pso_pool, cmd_pool);
        _thread_local.initialize(_globals.device);
    }

    void translateCommandList(VkCommandBuffer list, handle::command_list list_handle, vk_incomplete_state_cache* state_cache, std::byte* buffer, size_t buffer_size);

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
    vk_incomplete_state_cache* _state_cache = nullptr;
    VkCommandBuffer _cmd_list = nullptr;
    handle::command_list _cmd_list_handle = handle::null_command_list;

    // dynamic state
    struct
    {
        handle::pipeline_state pipeline_state;
        handle::resource index_buffer;
        handle::resource vertex_buffer;

        cc::array<handle::shader_view, limits::max_shader_arguments> shader_views;

        VkRenderPass raw_render_pass;
        VkFramebuffer raw_framebuffer;
        VkDescriptorSet raw_sampler_descriptor_set;
        VkPipelineLayout raw_pipeline_layout;

        cmd::begin_render_pass current_render_pass;


        void reset()
        {
            pipeline_state = handle::null_pipeline_state;
            index_buffer = handle::null_resource;
            vertex_buffer = handle::null_resource;

            raw_render_pass = nullptr;
            raw_framebuffer = nullptr;
            set_pipeline_layout(nullptr);
        }

        void set_pipeline_layout(VkPipelineLayout raw)
        {
            for (auto& sv : shader_views)
                sv = handle::null_shader_view;

            raw_pipeline_layout = raw;
            raw_sampler_descriptor_set = nullptr;
        }

    } _bound;
};

}
