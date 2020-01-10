#include "pso_pool.hh"

#include <iostream>

#include <phantasm-renderer/backend/d3d12/common/native_enum.hh>
#include <phantasm-renderer/backend/d3d12/common/util.hh>
#include <phantasm-renderer/backend/d3d12/pipeline_state.hh>

pr::backend::handle::pipeline_state pr::backend::d3d12::PipelineStateObjectPool::createPipelineState(pr::backend::arg::vertex_format vertex_format,
                                                                                                     pr::backend::arg::framebuffer_config const& framebuffer_format,
                                                                                                     pr::backend::arg::shader_argument_shapes shader_arg_shapes,
                                                                                                     bool has_root_constants,
                                                                                                     pr::backend::arg::shader_stages shader_stages,
                                                                                                     const pr::primitive_pipeline_config& primitive_config)
{
    root_signature* root_sig;
    unsigned pool_index;
    // Do things requiring synchronization first
    {
        auto lg = std::lock_guard(mMutex);
        root_sig = mRootSigCache.getOrCreate(*mDevice, shader_arg_shapes, has_root_constants, false);
        pool_index = mPool.acquire();
    }

    // Populate new node
    pso_node& new_node = mPool.get(pool_index);
    new_node.associated_root_sig = root_sig;

    {
        // Create PSO
        auto const vert_format_native = util::get_native_vertex_format(vertex_format.attributes);
        new_node.raw_pso = create_pipeline_state(*mDevice, root_sig->raw_root_sig, vert_format_native, framebuffer_format, shader_stages, primitive_config);
        util::set_object_name(new_node.raw_pso, "pool pso #%d", int(pool_index));
    }

    new_node.primitive_topology = util::to_native_topology(primitive_config.topology);

    return {static_cast<handle::index_t>(pool_index)};
}

pr::backend::handle::pipeline_state pr::backend::d3d12::PipelineStateObjectPool::createComputePipelineState(pr::backend::arg::shader_argument_shapes shader_arg_shapes,
                                                                                                            const pr::backend::arg::shader_stage& compute_shader,
                                                                                                            bool has_root_constants)
{
    root_signature* root_sig;
    unsigned pool_index;
    // Do things requiring synchronization first
    {
        auto lg = std::lock_guard(mMutex);
        root_sig = mRootSigCache.getOrCreate(*mDevice, shader_arg_shapes, has_root_constants, true);
        pool_index = mPool.acquire();
    }

    // Populate new node
    pso_node& new_node = mPool.get(pool_index);
    new_node.associated_root_sig = root_sig;
    new_node.primitive_topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

    // Create PSO
    new_node.raw_pso = create_compute_pipeline_state(*mDevice, root_sig->raw_root_sig, compute_shader);
    util::set_object_name(new_node.raw_pso, "pool compute pso #%d", int(pool_index));

    return {static_cast<handle::index_t>(pool_index)};
}

void pr::backend::d3d12::PipelineStateObjectPool::free(pr::backend::handle::pipeline_state ps)
{
    // TODO: dangle check

    // This requires no synchronization, as D3D12MA internally syncs
    pso_node& freed_node = mPool.get(static_cast<unsigned>(ps.index));
    freed_node.raw_pso->Release();

    {
        // This is a write access to the pool and must be synced
        auto lg = std::lock_guard(mMutex);
        mPool.release(static_cast<unsigned>(ps.index));
    }
}

void pr::backend::d3d12::PipelineStateObjectPool::initialize(ID3D12Device* device, unsigned max_num_psos)
{
    mDevice = device;
    mPool.initialize(max_num_psos);
    mRootSigCache.initialize(max_num_psos / 2); // almost arbitrary, revisit if this blows up
}

void pr::backend::d3d12::PipelineStateObjectPool::destroy()
{
    auto num_leaks = 0;
    mPool.iterate_allocated_nodes([&](pso_node& leaked_node, unsigned) {
        ++num_leaks;
        leaked_node.raw_pso->Release();
    });

    if (num_leaks > 0)
    {
        std::cout << "[pr][backend][d3d12] warning: leaked " << num_leaks << " handle::pipeline_state object" << (num_leaks == 1 ? "" : "s") << std::endl;
    }

    mRootSigCache.destroy();
}
