#include "pipeline_pool.hh"

#include <iostream>

#include <phantasm-renderer/backend/vulkan/common/util.hh>
#include <phantasm-renderer/backend/vulkan/pipeline_state.hh>

pr::backend::handle::pipeline_state pr::backend::vk::PipelinePool::createPipelineState(pr::backend::arg::vertex_format vertex_format,
                                                                                       pr::backend::arg::framebuffer_format framebuffer_format,
                                                                                       pr::backend::arg::shader_argument_shapes shader_arg_shapes,
                                                                                       pr::backend::arg::shader_stages shader_stages,
                                                                                       const pr::primitive_pipeline_config& primitive_config)
{
    pipeline_layout* layout;
    VkRenderPass render_pass;
    unsigned pool_index;
    // Do things requiring synchronization first
    {
        auto lg = std::lock_guard(mMutex);
        layout = mLayoutCache.getOrCreate(mDevice, shader_arg_shapes);
        render_pass = mRenderPassCache.getOrCreate(mDevice, framebuffer_format, primitive_config);
        pool_index = mPool.acquire();
    }

    // Populate new node
    pso_node& new_node = mPool.get(pool_index);
    new_node.associated_pipeline_layout = layout;
    new_node.associated_render_pass = render_pass;

    {
        // Create VkPipeline
        auto const vert_format_native = util::get_native_vertex_format(vertex_format.attributes);
        new_node.raw_pipeline = create_pipeline(mDevice, new_node.associated_render_pass, new_node.associated_pipeline_layout->raw_layout,
                                                shader_stages, primitive_config, vert_format_native, vertex_format.vertex_size_bytes);
    }

    //    new_node.primitive_topology = util::to_native_topology(primitive_config.topology);

    return {static_cast<handle::index_t>(pool_index)};
}

void pr::backend::vk::PipelinePool::free(pr::backend::handle::pipeline_state ps)
{
    // TODO: dangle check

    // This requires no synchronization, as VMA internally syncs
    pso_node& freed_node = mPool.get(static_cast<unsigned>(ps.index));
    vkDestroyPipeline(mDevice, freed_node.raw_pipeline, nullptr);

    {
        // This is a write access to the pool and must be synced
        auto lg = std::lock_guard(mMutex);
        mPool.release(static_cast<unsigned>(ps.index));
    }
}

void pr::backend::vk::PipelinePool::initialize(VkDevice device, unsigned max_num_psos)
{
    mDevice = device;
    mPool.initialize(max_num_psos);

    // almost arbitrary, but this is not a hard max, just reserving
    mLayoutCache.initialize(max_num_psos / 2);
    mRenderPassCache.initialize(max_num_psos / 2);
}

void pr::backend::vk::PipelinePool::destroy()
{
    auto num_leaks = 0;
    mPool.iterate_allocated_nodes([&](pso_node& leaked_node) {
        ++num_leaks;
        vkDestroyPipeline(mDevice, leaked_node.raw_pipeline, nullptr);
    });

    if (num_leaks > 0)
    {
        std::cout << "[pr][backend][vk] warning: leaked " << num_leaks << " handle::pipeline_state object" << (num_leaks == 1 ? "" : "s") << std::endl;
    }

    mLayoutCache.destroy(mDevice);
    mRenderPassCache.destroy(mDevice);
}
