#pragma once

#include <cstdint>

#include <clean-core/assert.hh>

namespace pr::backend::detail
{
// This is the typical ring buffer, it is used by resources that will be reused.
// For example the command Lists, the 'dynamic' constant buffers, etc..
//
class Ring
{
public:
    void initialize(uint32_t total_size) { mTotalSize = total_size; }

    uint32_t getSize() const { return mAllocatedSize; }
    uint32_t getHead() const { return mHead; }
    uint32_t getTail() const { return (mHead + mAllocatedSize) % mTotalSize; }

    // helper to avoid allocating chunks that wouldn't fit contiguously in the ring
    uint32_t paddingToAvoidCrossover(uint32_t size) const
    {
        auto const tail = getTail();
        if ((tail + size) > mTotalSize)
            return (mTotalSize - tail);
        else
            return 0;
    }

    bool alloc(uint32_t size, uint32_t* out_start)
    {
        if (mAllocatedSize + size <= mTotalSize)
        {
            if (out_start)
                *out_start = getTail();

            mAllocatedSize += size;
            return true;
        }

        CC_ASSERT(false);
        return false;
    }

    bool free(uint32_t size)
    {
        if (mAllocatedSize > size)
        {
            mHead = (mHead + size) % mTotalSize;
            mAllocatedSize -= size;
            return true;
        }
        return false;
    }

private:
    uint32_t mTotalSize = 0;
    uint32_t mHead = 0;
    uint32_t mAllocatedSize = 0;
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
        mNumBackbuffers = numberOfBackBuffers;

        // init mem per frame tracker
        for (int i = 0; i < 4; i++)
            mAllocatedMemPerFrame[i] = 0;

        mMemory.initialize(memTotalSize);
    }

    bool alloc(uint32_t size, uint32_t* out_start)
    {
        uint32_t padding = mMemory.paddingToAvoidCrossover(size);
        if (padding > 0)
        {
            mMemAllocatedInFrame += padding;

            if (mMemory.alloc(padding, nullptr) == false) // alloc chunk to avoid crossover, ignore offset
            {
                return false; // no mem, cannot allocate padding
            }
        }

        if (mMemory.alloc(size, out_start) == true)
        {
            mMemAllocatedInFrame += size;
            return true;
        }
        return false;
    }

    void onBeginFrame()
    {
        mAllocatedMemPerFrame[mBackbufferIndex] = mMemAllocatedInFrame;
        mMemAllocatedInFrame = 0;

        ++mBackbufferIndex;
        if (mBackbufferIndex >= mNumBackbuffers)
            mBackbufferIndex -= mNumBackbuffers;

        // free all the entries for the oldest buffer in one go
        uint32_t memToFree = mAllocatedMemPerFrame[mBackbufferIndex];
        mMemory.free(memToFree);
    }

private:
    // internal ring buffer
    Ring mMemory;

    // external ring buffer
    uint32_t mBackbufferIndex = 0;
    uint32_t mNumBackbuffers = 0;

    uint32_t mMemAllocatedInFrame = 0;
    uint32_t mAllocatedMemPerFrame[4];
};
}
