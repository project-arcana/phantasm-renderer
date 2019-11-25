#pragma once

#include <cstdint>

#include <clean-core/vector.hh>

#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

namespace pr::backend::vk
{
class Device;

class UploadHeap
{
public:
    void initialize(Device* device, size_t size);
    ~UploadHeap();

    [[nodiscard]] uint8_t* suballocate(size_t size, size_t align);

    template <class T>
    [[nodiscard]] uint8_t* suballocate()
    {
        return suballocate(sizeof(T), alignof(T));
    }

    /// suballocate, but calls flushAndFinish internally if overcomitted
    [[nodiscard]] uint8_t* suballocateAllowRetry(size_t size, size_t align);

    /// copy an allocation received by suballocate() to a destination buffer
    void copyAllocationToBuffer(VkBuffer dest_buffer, uint8_t* src_allocation, size_t size);

    /// copy an allocation received by suballocate() to a destination image
    void copyAllocationToImage(VkImage dest_image, uint8_t* src_allocation, uint32_t width, uint32_t height, uint32_t mip = 0, uint32_t layer = 0);

    /// insert a transition barrier for the given resource upon next flush
    /// also takes care of the state cache
    // void barrierResourceOnFlush(D3D12MA::Allocation* allocation, D3D12_RESOURCE_STATES after);

    void flush();

    /// flush all pending outgoing copy operations and barriers, free the internal upload heap
    void flushAndFinish();

    [[nodiscard]] uint8_t* getBasePointer() const { return mDataBegin; }
    [[nodiscard]] VkBuffer getResource() const { return mUploadHeap; }
    [[nodiscard]] VkCommandBuffer getCommandBuffer() const { return mCommandBuffer; }


private:
    Device* mDevice = nullptr;

    VkBuffer mUploadHeap;
    VkDeviceMemory mUploadHeapMemory;

    VkCommandBuffer mCommandBuffer;
    VkCommandPool mCommandPool;

    VkFence mFence;

    uint8_t* mDataCurrent = nullptr; ///< current position of upload heap
    uint8_t* mDataBegin = nullptr;   ///< starting position of upload heap
    uint8_t* mDataEnd = nullptr;     ///< ending position of upload heap
};
}
