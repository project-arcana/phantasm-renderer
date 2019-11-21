#pragma once

#include <unordered_map>

#include <phantasm-renderer/backend/arguments.hh>
#include <phantasm-renderer/backend/detail/hash.hh>

#include <phantasm-renderer/backend/d3d12/common/d3d12_fwd.hh>
#include <phantasm-renderer/backend/d3d12/root_signature.hh>

namespace pr::backend::d3d12
{

/// TODO, plan changed:
/// there won't be an explicit handle::framebuffer,
/// but it won't just be handle::resource for RTVs either
///
/// There are two places where "resource views" are API exposed:
/// handle::shader_view
///     (SRVs + UAVs for shaders)
///
/// TBD::SetRenderTargets bytecode within command list
///     (RTVs + DSV for render targets)
///
/// However within this command, there's nothing but the handle::resource
/// and the "how to view" info. We create small RTV + DSV heaps
/// PER command list, which act as linear allocators and create the
/// descriptors (remember, not GPU-visible) on the fly
///
/// Jesse Natalie: CPU-only descriptors have ZERO lifetime requirements and can be invalidated before
/// the command list is even closed. This simplifies management for the linear allocators.
///
/// The data structure for this linear allocator could be placed here,
/// and must be referenced in CommandListPool::cmd_list_node
///

//class RootSignatureCache
//{
//public:
//    using key_t = arg::shader_argument_shapes;

//    void initialize(unsigned size_estimate = 50);
//    void destroy();

//    /// receive an existing root signature matching the shape, or create a new one
//    /// returns a pointer (which remains stable, §23.2.5/13 C++11)
//    [[nodiscard]] root_signature_ll* getOrCreate(ID3D12Device& device, key_t arg_shapes);

//    /// destroys all elements inside, and clears the map
//    void reset();

//private:
//    std::unordered_map<key_t, root_signature_ll, hash::compute_functor> mCache;
//};

}
