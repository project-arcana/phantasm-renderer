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

    [[nodiscard]] handle::accel_struct createTopLevelAS(unsigned num_instances);

    void free(handle::accel_struct as);
    void free(cc::span<handle::accel_struct const> as);

public:
    struct accel_struct_node
    {
        VkAccelerationStructureNV raw_as;
        std::byte* buffer_instances_map;
        handle::resource buffer_as;
        handle::resource buffer_scratch;
        handle::resource buffer_instances;
    };

    /// Geometry instance, with the layout expected by VK_NV_ray_tracing
    struct vulkan_geometry_instance
    {
        /// Transform matrix, containing only the top 3 rows
        float transform[12];
        /// Instance index
        uint32_t instanceId : 24;
        /// Visibility mask
        uint32_t mask : 8;
        /// Index of the hit group which will be invoked when a ray hits the instance
        uint32_t instanceOffset : 24;
        /// Instance flags, such as culling
        uint32_t flags : 8;
        /// Opaque handle of the bottom-level acceleration structure
        uint64_t accelerationStructureHandle;
    };

    static_assert(sizeof(vulkan_geometry_instance) == 64, "vulkan_geometry_instance structure compiles to incorrect size");

private:
    handle::accel_struct acquireAccelStruct(VkAccelerationStructureNV raw_as,
                                            handle::resource buffer_as,
                                            handle::resource buffer_scratch,
                                            handle::resource buffer_instances = handle::null_resource);

    void internalFree(accel_struct_node& node);

private:
    VkDevice mDevice;
    ResourcePool* mResourcePool;

    backend::detail::linked_pool<accel_struct_node, unsigned> mPool;

    std::mutex mMutex;
};

}
