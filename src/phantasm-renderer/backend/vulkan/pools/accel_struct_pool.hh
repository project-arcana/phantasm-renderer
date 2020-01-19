#pragma once

#include <mutex>

#include <clean-core/span.hh>
#include <clean-core/vector.hh>

#include <phantasm-renderer/backend/arguments.hh>
#include <phantasm-renderer/backend/detail/linked_pool.hh>
#include <phantasm-renderer/backend/types.hh>

#include <phantasm-renderer/backend/vulkan/loader/vulkan_fwd.hh>

namespace pr::backend::vk
{
class ResourcePool;

class AccelStructPool
{
public:
    [[nodiscard]] handle::accel_struct createBottomLevelAS(cc::span<arg::blas_element const> elements, accel_struct_build_flags_t flags);

    [[nodiscard]] handle::accel_struct createTopLevelAS(unsigned num_instances);

    void free(handle::accel_struct as);
    void free(cc::span<handle::accel_struct const> as);

public:
    void initialize(VkDevice device, ResourcePool* res_pool, unsigned max_num_accel_structs);
    void destroy();


public:
    struct accel_struct_node
    {
        VkAccelerationStructureNV raw_as;
        uint64_t raw_as_handle;
        std::byte* buffer_instances_map;
        handle::resource buffer_as;
        handle::resource buffer_scratch;
        handle::resource buffer_instances;
        accel_struct_build_flags_t flags;
        cc::vector<VkGeometryNV> geometries;
    };

public:
    accel_struct_node& getNode(handle::accel_struct as);

private:
    handle::accel_struct acquireAccelStruct(VkAccelerationStructureNV raw_as,
                                            accel_struct_build_flags_t flags,
                                            handle::resource buffer_as,
                                            handle::resource buffer_scratch,
                                            handle::resource buffer_instances = handle::null_resource);

    void moveGeometriesToAS(handle::accel_struct as, cc::vector<VkGeometryNV>&& geometries);

    void internalFree(accel_struct_node& node);

private:
    VkDevice mDevice = nullptr;
    ResourcePool* mResourcePool = nullptr;

    backend::detail::linked_pool<accel_struct_node, unsigned> mPool;

    std::mutex mMutex;
};

}