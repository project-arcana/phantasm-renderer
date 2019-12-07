#pragma once

#include <mutex>

#include <clean-core/capped_vector.hh>

#include <phantasm-renderer/backend/arguments.hh>
#include <phantasm-renderer/backend/detail/linked_pool.hh>
#include <phantasm-renderer/backend/types.hh>
#include <phantasm-renderer/primitive_pipeline_config.hh>

#include <phantasm-renderer/backend/vulkan/loader/volk.hh>
#include <phantasm-renderer/backend/vulkan/resources/descriptor_allocator.hh>

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

        // info stored which is required for creating render passes
        cc::capped_vector<format, limits::max_render_targets> rt_formats;
        int num_msaa_samples;
    };

public:
    // internal API

    void initialize(VkDevice device, unsigned max_num_psos);
    void destroy();

    [[nodiscard]] pso_node const& get(handle::pipeline_state ps) const { return mPool.get(static_cast<unsigned>(ps.index)); }

    [[nodiscard]] VkRenderPass getOrCreateRenderPass(pso_node const& node, cmd::begin_render_pass const& brp_cmd);

private:
    VkDevice mDevice;
    PipelineLayoutCache mLayoutCache;
    RenderPassCache mRenderPassCache;
    DescriptorAllocator mDescriptorAllocator;
    backend::detail::linked_pool<pso_node, unsigned> mPool;
    std::mutex mMutex;
};

}
