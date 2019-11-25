#pragma once

#include <unordered_map>

#include <phantasm-renderer/backend/arguments.hh>
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
    using key_t = arg::shader_argument_shapes;

    void initialize(unsigned size_estimate = 50);
    void destroy();

    /// receive an existing root signature matching the shape, or create a new one
    /// returns a pointer (which remains stable, §23.2.5/13 C++11)
    [[nodiscard]] root_signature* getOrCreate(ID3D12Device& device, key_t arg_shapes);

    /// destroys all elements inside, and clears the map
    void reset();

private:
    std::unordered_map<key_t, root_signature, hash::compute_functor> mCache;
};

}
