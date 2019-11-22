#pragma once

#include <mutex>

#include <phantasm-renderer/backend/arguments.hh>
#include <phantasm-renderer/backend/types.hh>
#include <phantasm-renderer/primitive_pipeline_config.hh>

#include <phantasm-renderer/backend/d3d12/common/d3d12_fwd.hh>
#include <phantasm-renderer/backend/detail/linked_pool.hh>

#include "root_sig_cache.hh"

namespace pr::backend::d3d12
{
/// The high-level allocator for PSOs and root signatures
/// Synchronized
class PipelineStateObjectPool
{
public:
    // frontend-facing API

    [[nodiscard]] handle::pipeline_state createPipelineState(arg::vertex_format vertex_format,
                                                             arg::framebuffer_format framebuffer_format,
                                                             arg::shader_argument_shapes shader_arg_shapes,
                                                             arg::shader_stages shader_stages,
                                                             pr::primitive_pipeline_config const& primitive_config);

    void free(handle::pipeline_state ps);

public:
    struct pso_node
    {
        ID3D12PipelineState* raw_pso;
        root_signature* associated_root_sig;
        D3D12_PRIMITIVE_TOPOLOGY primitive_topology;
    };

public:
    // internal API

    void initialize(ID3D12Device* device, unsigned max_num_psos);
    void destroy();

    [[nodiscard]] pso_node const& get(handle::pipeline_state ps) const { return mPool.get(static_cast<unsigned>(ps.index)); }

private:
    ID3D12Device* mDevice;
    RootSignatureCache mRootSigCache;
    backend::detail::linked_pool<pso_node, unsigned> mPool;
    std::mutex mMutex;
};

}
