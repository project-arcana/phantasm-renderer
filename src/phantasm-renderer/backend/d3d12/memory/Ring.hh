#pragma once

#include <cstdint>

#include <clean-core/assert.hh>

namespace pr::backend::d3d12
{
// This is the typical ring buffer, it is used by resources that will be reused.
// For example the command Lists, the 'dynamic' constant buffers, etc..
//
class Ring
{
public:
    ~Ring() { Free(m_AllocatedSize); }

    void initialize(uint32_t total_size) { m_TotalSize = total_size; }

    uint32_t GetSize() const { return m_AllocatedSize; }
    uint32_t GetHead() const { return m_Head; }
    uint32_t GetTail() const { return (m_Head + m_AllocatedSize) % m_TotalSize; }

    // helper to avoid allocating chunks that wouldn't fit contiguously in the ring
    uint32_t PaddingToAvoidCrossOver(uint32_t size) const
    {
        auto const tail = GetTail();
        if ((tail + size) > m_TotalSize)
            return (m_TotalSize - tail);
        else
            return 0;
    }

    bool Alloc(uint32_t size, uint32_t* pOut)
    {
        if (m_AllocatedSize + size <= m_TotalSize)
        {
            if (pOut)
                *pOut = GetTail();

            m_AllocatedSize += size;
            return true;
        }

        CC_ASSERT(false);
        return false;
    }

    bool Free(uint32_t size)
    {
        if (m_AllocatedSize > size)
        {
            m_Head = (m_Head + size) % m_TotalSize;
            m_AllocatedSize -= size;
            return true;
        }
        return false;
    }

private:
    uint32_t m_TotalSize = 0;
    uint32_t m_Head = 0;
    uint32_t m_AllocatedSize = 0;
};

//
// This class can be thought as ring buffer inside a ring buffer. The outer ring is for ,
// the frames and the internal one is for the resources that were allocated for that frame.
// The size of the outer ring is typically the number of back buffers.
//
// When the outer ring is full, for the next allocation it automatically frees the entries
// of the oldest frame and makes those entries available for the next frame. This happens
// when you call 'OnBeginFrame()'
//
class RingWithTabs
{
public:
    void initialize(uint32_t numberOfBackBuffers, uint32_t memTotalSize)
    {
        m_numberOfBackBuffers = numberOfBackBuffers;

        // init mem per frame tracker
        for (int i = 0; i < 4; i++)
            m_allocatedMemPerBackBuffer[i] = 0;

        m_mem.initialize(memTotalSize);
    }

    bool Alloc(uint32_t size, uint32_t* pOut)
    {
        uint32_t padding = m_mem.PaddingToAvoidCrossOver(size);
        if (padding > 0)
        {
            m_memAllocatedInFrame += padding;

            if (m_mem.Alloc(padding, nullptr) == false) // alloc chunk to avoid crossover, ignore offset
            {
                return false; // no mem, cannot allocate apdding
            }
        }

        if (m_mem.Alloc(size, pOut) == true)
        {
            m_memAllocatedInFrame += size;
            return true;
        }
        return false;
    }

    void OnBeginFrame()
    {
        m_allocatedMemPerBackBuffer[m_backBufferIndex] = m_memAllocatedInFrame;
        m_memAllocatedInFrame = 0;

        m_backBufferIndex = (m_backBufferIndex + 1) % m_numberOfBackBuffers;

        // free all the entries for the oldest buffer in one go
        uint32_t memToFree = m_allocatedMemPerBackBuffer[m_backBufferIndex];
        m_mem.Free(memToFree);
    }

private:
    // internal ring buffer
    Ring m_mem;

    // this is the external ring buffer (I could have reused the Ring class though)
    uint32_t m_backBufferIndex = 0;
    uint32_t m_numberOfBackBuffers = 0;

    uint32_t m_memAllocatedInFrame = 0;
    uint32_t m_allocatedMemPerBackBuffer[4];
};
}
