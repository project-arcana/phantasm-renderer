#pragma once

#include <mutex>

#include <phantasm-renderer/backend/arguments.hh>
#include <phantasm-renderer/backend/detail/linked_pool.hh>
#include <phantasm-renderer/backend/types.hh>
#include <phantasm-renderer/primitive_pipeline_config.hh>

#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

#include "pipeline_layout_cache.hh"
#include "render_pass_cache.hh"

namespace pr::backend::vk
{
/// The high-level allocator for PSOs and root signatures
/// Synchronized
class PipelinePool
{
public:
    // frontend-facing API

    [[nodiscard]] handle::pipeline_state createPipelineState(arg::vertex_format vertex_format,
                                                             arg::framebuffer_format framebuffer_format,
                                                             arg::shader_argument_shapes shader_arg_shapes,
                                                             arg::shader_sampler_configs shader_samplers,
                                                             arg::shader_stages shader_stages,
                                                             pr::primitive_pipeline_config const& primitive_config);

    void free(handle::pipeline_state ps);

public:
    struct pso_node
    {
        VkPipeline raw_pipeline;
        pipeline_layout* associated_pipeline_layout;
        VkRenderPass associated_render_pass;
        //        D3D12_PRIMITIVE_TOPOLOGY primitive_topology;
    };

public:
    // internal API

    void initialize(VkDevice device, unsigned max_num_psos);
    void destroy();

    [[nodiscard]] pso_node const& get(handle::pipeline_state ps) const { return mPool.get(static_cast<unsigned>(ps.index)); }

private:
    VkDevice mDevice;
    PipelineLayoutCache mLayoutCache;
    RenderPassCache mRenderPassCache;
    backend::detail::linked_pool<pso_node, unsigned> mPool;
    std::mutex mMutex;
};

}
