#include "accel_struct_pool.hh"

#include <clean-core/vector.hh>

#include <phantasm-renderer/backend/vulkan/common/verify.hh>
#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

#include "resource_pool.hh"

pr::backend::handle::accel_struct pr::backend::vk::AccelStructPool::createBottomLevelAS(cc::span<const pr::backend::vk::AccelStructPool::blas_element> elements)
{
    cc::vector<VkGeometryNV> element_geometries;
    element_geometries.reserve(elements.size());

    for (auto const& elem : elements)
    {
        auto const& vert_info = mResourcePool->getBufferInfo(elem.vertex_buffer);

        VkGeometryNV& egeom = element_geometries.emplace_back();
        egeom = {};
        egeom.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
        egeom.pNext = nullptr;
        egeom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
        egeom.geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
        egeom.geometry.triangles.vertexData = vert_info.raw_buffer;
        egeom.geometry.triangles.vertexOffset = elem.vertex_offset * vert_info.stride;
        egeom.geometry.triangles.vertexCount = elem.num_vertices;
        egeom.geometry.triangles.vertexStride = vert_info.stride;
        egeom.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;

        if (elem.index_buffer.is_valid())
        {
            auto const index_stride = mResourcePool->getBufferInfo(elem.index_buffer).stride;

            egeom.geometry.triangles.indexData = mResourcePool->getRawBuffer(elem.index_buffer);
            egeom.geometry.triangles.indexCount = elem.num_indices;
            egeom.geometry.triangles.indexOffset = index_stride * elem.index_offset;
            egeom.geometry.triangles.indexType = index_stride == sizeof(uint16_t) ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
        }
        else
        {
            egeom.geometry.triangles.indexData = nullptr;
            egeom.geometry.triangles.indexCount = 0;
            egeom.geometry.triangles.indexOffset = 0;
            egeom.geometry.triangles.indexType = VK_INDEX_TYPE_NONE_NV;
        }

        egeom.geometry.aabbs.sType = VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV;
        egeom.flags = elem.is_opaque ? VK_GEOMETRY_OPAQUE_BIT_NV : 0;
    }

    VkAccelerationStructureCreateInfoNV as_create_info = {};
    as_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
    as_create_info.pNext = nullptr;
    as_create_info.info = {};
    as_create_info.info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    as_create_info.info.pNext = nullptr;
    as_create_info.info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
    as_create_info.info.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV;
    as_create_info.info.instanceCount = 0;
    as_create_info.info.geometryCount = static_cast<uint32_t>(element_geometries.size());
    as_create_info.info.pGeometries = element_geometries.data();
    as_create_info.compactedSize = 0;

    return acquireAccelStruct(&as_create_info);
}

void pr::backend::vk::AccelStructPool::free(pr::backend::handle::accel_struct as)
{
    if (!as.is_valid())
        return;

    accel_struct_node& freed_node = mPool.get(static_cast<unsigned>(as.index));
    internalFree(freed_node);

    {
        auto lg = std::lock_guard(mMutex);
        mPool.release(static_cast<unsigned>(as.index));
    }
}

void pr::backend::vk::AccelStructPool::free(cc::span<const pr::backend::handle::accel_struct> as_span)
{
    auto lg = std::lock_guard(mMutex);

    for (auto as : as_span)
    {
        if (as.is_valid())
        {
            accel_struct_node& freed_node = mPool.get(static_cast<unsigned>(as.index));
            internalFree(freed_node);
            mPool.release(static_cast<unsigned>(as.index));
        }
    }
}

pr::backend::handle::accel_struct pr::backend::vk::AccelStructPool::acquireAccelStruct(const VkAccelerationStructureCreateInfoNV* create_info)
{
    unsigned res;
    {
        auto lg = std::lock_guard(mMutex);
        res = mPool.acquire();
    }

    accel_struct_node& new_node = mPool.get(res);
    new_node.raw_as = nullptr;

    PR_VK_VERIFY_SUCCESS(vkCreateAccelerationStructureNV(mDevice, create_info, nullptr, &new_node.raw_as));
    return {static_cast<handle::index_t>(res)};
}

void pr::backend::vk::AccelStructPool::internalFree(pr::backend::vk::AccelStructPool::accel_struct_node& node)
{
    vkDestroyAccelerationStructureNV(mDevice, node.raw_as, nullptr);
}
