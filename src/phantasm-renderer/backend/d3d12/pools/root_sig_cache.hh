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
    struct key_t
    {
        arg::shader_argument_shapes arg_shapes;
        arg::shader_sampler_configs arg_samplers;
    };

    void initialize(unsigned size_estimate = 50);
    void destroy();

    /// receive an existing root signature matching the shape, or create a new one
    /// returns a pointer (which remains stable, §23.2.5/13 C++11)
    [[nodiscard]] root_signature* getOrCreate(ID3D12Device& device, key_t arg_shapes);

    /// destroys all elements inside, and clears the map
    void reset();

private:
    struct key_hasher_functor
    {
        std::size_t operator()(key_t const& v) const noexcept
        {
            return hash::detail::hash_combine(hash::compute(v.arg_shapes), hash::compute(v.arg_samplers));
        }
    };

    struct key_equal_op
    {
        bool operator()(key_t const& lhs, key_t const& rhs) const noexcept
        {
            return arg::operator==(lhs.arg_shapes, rhs.arg_shapes) && arg::operator==(lhs.arg_samplers, rhs.arg_samplers);
        }
    };

    std::unordered_map<key_t, root_signature, key_hasher_functor, key_equal_op> mCache;
};

}
