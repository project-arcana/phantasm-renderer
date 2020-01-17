#pragma once

#include <mutex>

#include <clean-core/span.hh>

#include <phantasm-renderer/backend/detail/linked_pool.hh>
#include <phantasm-renderer/backend/types.hh>

#include <phantasm-renderer/backend/vulkan/loader/vulkan_fwd.hh>

namespace pr::backend::vk
{
class ResourcePool;

class AccelStructPool
{
public:
    struct blas_element
    {
        handle::resource vertex_buffer;
        handle::resource index_buffer;
        unsigned num_vertices;
        unsigned vertex_offset; ///< offset in number of vertices
        unsigned num_indices;
        unsigned index_offset; ///< offset in number of indices
        bool is_opaque;
    };

    [[nodiscard]] handle::accel_struct createBottomLevelAS(cc::span<blas_element const> elements);

    void free(handle::accel_struct as);
    void free(cc::span<handle::accel_struct const> as);

public:
    struct accel_struct_node
    {
        VkAccelerationStructureNV raw_as;
    };

private:
    handle::accel_struct acquireAccelStruct(VkAccelerationStructureCreateInfoNV const* create_info);

    void internalFree(accel_struct_node& node);

private:
    VkDevice mDevice;
    ResourcePool* mResourcePool;

    backend::detail::linked_pool<accel_struct_node, unsigned> mPool;

    std::mutex mMutex;
};

}
