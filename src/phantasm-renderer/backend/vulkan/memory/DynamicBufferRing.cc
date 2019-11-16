#include "DynamicBufferRing.hh"

#include <clean-core/assert.hh>

#include <phantasm-renderer/backend/detail/byte_util.hh>
#include <phantasm-renderer/backend/vulkan/Device.hh>


namespace pr::backend::vk
{
void DynamicBufferRing::initialize(Device* device, Allocator* allocator, uint32_t num_backbuffers, uint32_t total_size_in_bytes)
{
    CC_ASSERT(mDevice == nullptr);
    mDevice = device;
    mAllocator = allocator;

    mTotalSize = mem::align_offset(total_size_in_bytes, 256u);

    mMemoryRing.initialize(num_backbuffers, mTotalSize);

    mBuffer = mAllocator->allocCPUtoGPUBuffer(mTotalSize, //
                                              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, //
                                              reinterpret_cast<void**>(&mData) //
    );
}

DynamicBufferRing::~DynamicBufferRing()
{
    if (mAllocator)
    {
        mAllocator->unmapCPUtoGPUBuffer(mBuffer);
        mAllocator->free(mBuffer);
    }
}

void DynamicBufferRing::onBeginFrame() { mMemoryRing.onBeginFrame(); }

bool DynamicBufferRing::allocConstantBuffer(uint32_t size, void*& map_ptr, VkDescriptorBufferInfo& out_info)
{
    size = mem::align_offset(size, 256u);

    uint32_t memOffset;
    if (mMemoryRing.alloc(size, &memOffset) == false)
    {
        CC_ASSERT(false && "DynamicBufferRing overcommited, increase size");
        return false;
    }

    map_ptr = static_cast<void*>(mData + memOffset);

    out_info.buffer = mBuffer.buffer;
    out_info.offset = memOffset;
    out_info.range = size;

    return true;
}

bool DynamicBufferRing::allocVertexIndexBuffer(uint32_t num_elements, uint32_t element_size_bytes, void*& map_ptr, VkDescriptorBufferInfo& out_info)
{
    return allocConstantBuffer(num_elements * element_size_bytes, map_ptr, out_info);
}

void DynamicBufferRing::setDescriptorSet(uint32_t index, uint32_t size, VkDescriptorSet descriptorSet)
{
    VkDescriptorBufferInfo out = {};
    out.buffer = mBuffer.buffer;
    out.offset = 0;
    out.range = size;

    VkWriteDescriptorSet write;
    write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;
    write.dstSet = descriptorSet;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    write.pBufferInfo = &out;
    write.dstArrayElement = 0;
    write.dstBinding = index;

    vkUpdateDescriptorSets(mDevice->getDevice(), 1, &write, 0, nullptr);
}
}
