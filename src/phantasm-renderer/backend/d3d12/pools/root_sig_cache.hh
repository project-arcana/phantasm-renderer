#pragma once

#include <phantasm-renderer/backend/arguments.hh>
#include <phantasm-renderer/backend/detail/cache_map.hh>
#include <phantasm-renderer/backend/detail/hash.hh>

#include <phantasm-renderer/backend/d3d12/common/d3d12_fwd.hh>
#include <phantasm-renderer/backend/d3d12/root_signature.hh>

namespace pr::backend::d3d12
{
/// Persistent cache for root signatures
/// Unsynchronized, only used inside of pso pool
class RootSignatureCache
{
public:
    struct key_t
    {
        arg::shader_argument_shapes arg_shapes;
        bool has_root_constants;
        bool is_compute;
    };

    void initialize(unsigned max_num_root_sigs);
    void destroy();

    /// receive an existing root signature matching the shape, or create a new one
    /// returns a pointer which remains stable
    [[nodiscard]] root_signature* getOrCreate(ID3D12Device& device, arg::shader_argument_shapes arg_shapes, bool has_root_constants, root_signature_type type);

    /// destroys all elements inside, and clears the map
    void reset();

private:
    static cc::hash_t hashKey(arg::shader_argument_shapes arg_shapes, bool has_root_constants, root_signature_type type)
    {
        return cc::make_hash(hash::compute(arg_shapes), type, has_root_constants);
    }

    backend::detail::cache_map<root_signature> mCache;
};

}
