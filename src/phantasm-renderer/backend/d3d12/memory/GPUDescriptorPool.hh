#pragma once
#ifdef PR_BACKEND_D3D12

/*
 *  Adapted from DynamicDescriptorHeap.h, in turn based on MS MiniEngine
 *  See: https://github.com/Microsoft/DirectX-Graphics-Samples
 *  Original license:
 *
 *  Copyright(c) 2018 Jeremiah van Oosten
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files(the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions :
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 */

#include <clean-core/array.hh>
#include <clean-core/typedefs.hh>
#include <clean-core/vector.hh>

#include <phantasm-renderer/backend/d3d12/common/d3dx12.hh>
#include <phantasm-renderer/backend/d3d12/common/shared_com_ptr.hh>

namespace pr::backend::d3d12
{
class CommandList
{
public:
    void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE, ID3D12DescriptorHeap*);
    ID3D12GraphicsCommandList& GetGraphicsCommandList() const;
};

class RootSignature
{
public:
    cc::uint32 GetDescriptorTableBitMask(D3D12_DESCRIPTOR_HEAP_TYPE heapType) const;
    int GetNumDescriptors(DWORD rootIndex) const;
    D3D12_ROOT_SIGNATURE_DESC const& GetRootSignatureDesc() const;
};

/// GPU visible descriptor heap that allows for staging of CPU visible descriptors that need
/// to be uploaded before a Draw or Dispatch command is executed
class GPUDescriptorPool
{
public:
    explicit GPUDescriptorPool(ID3D12Device& device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, int numDescriptorsPerHeap = 1024u);

    GPUDescriptorPool(GPUDescriptorPool const&) = delete;
    GPUDescriptorPool(GPUDescriptorPool&&) noexcept = delete;
    GPUDescriptorPool& operator=(GPUDescriptorPool const&) = delete;
    GPUDescriptorPool& operator=(GPUDescriptorPool&&) noexcept = delete;

    ~GPUDescriptorPool();
    /**
     * Stages a contiguous range of CPU visible descriptors.
     * Descriptors are not copied to the GPU visible descriptor heap until
     * the CommitStagedDescriptors function is called.
     */
    void stageDescriptors(unsigned rootParameterIndex, int offset, int numDescriptors, D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptors);

    /**
     * Copy all of the staged descriptors to the GPU visible descriptor heap and
     * bind the descriptor heap and the descriptor tables to the command list.
     * The passed-in function object is used to set the GPU visible descriptors
     * on the command list. Two possible functions are:
     *   * Before a draw    : ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable
     *   * Before a dispatch: ID3D12GraphicsCommandList::SetComputeRootDescriptorTable
     *
     * Since the DynamicDescriptorHeap can't know which function will be used, it must
     * be passed as an argument to the function.
     */
    void commitStagedDescriptorsForDraw(CommandList& commandList) { return commitStagedDescriptors<false>(commandList); }
    void commitStagedDescriptorsForDispatch(CommandList& commandList) { return commitStagedDescriptors<true>(commandList); }

    /**
     * Copies a single CPU visible descriptor to a GPU visible descriptor heap.
     * This is useful for the
     *   * ID3D12GraphicsCommandList::ClearUnorderedAccessViewFloat
     *   * ID3D12GraphicsCommandList::ClearUnorderedAccessViewUint
     * methods which require both a CPU and GPU visible descriptors for a UAV
     * resource.
     *
     * @param commandList The command list is required in case the GPU visible
     * descriptor heap needs to be updated on the command list.
     * @param cpuDescriptor The CPU descriptor to copy into a GPU visible
     * descriptor heap.
     *
     * @return The GPU visible descriptor.
     */
    [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE copyDescriptor(CommandList& comandList, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor);

    /**
     * Parse the root signature to determine which root parameters contain
     * descriptor tables and determine the number of descriptors needed for
     * each table.
     */
    void parseRootSignature(RootSignature const& rootSignature);

    /**
     * Reset used descriptors. This should only be done if any descriptors
     * that are being referenced by a command list has finished executing on the
     * command queue.
     */
    void reset();

private:
    /// Returns a pooled descriptor heap, or creates a new one
    [[nodiscard]] ID3D12DescriptorHeap* acquireHeap();

    /// Returns the number of stale descriptors that need to be copied to GPU visible descriptor heap
    [[nodiscard]] int getNumStaleDescriptors() const;

    /// Prepapares internal state for a commit, returns true iff the commit is to be executed
    [[nodiscard]] bool prepareDescriptorCommit(CommandList& commandList);

    template <bool compute_mode>
    void commitStagedDescriptors(CommandList& commandList);

    /**
     * The maximum number of descriptor tables per root signature.
     * A 32-bit mask is used to keep track of the root parameter indices that
     * are descriptor tables.
     */
    static constexpr cc::uint32 max_descriptor_tables = 32u;

    /// Represents a descriptor table entry in the root signature
    struct descriptor_table_cache
    {
        /// The number of descriptors in this descriptor table.
        int size = 0;
        /// The pointer to the descriptor in the descriptor handle cache.
        D3D12_CPU_DESCRIPTOR_HANDLE* base_descriptor = nullptr;

        void reset()
        {
            size = 0;
            base_descriptor = nullptr;
        }
    };

private:
    ID3D12Device& mDevice;
    D3D12_DESCRIPTOR_HEAP_TYPE const mDescriptorHeapType;

    /// The number of descriptors to allocate in new GPU visible descriptor heaps
    int const mNumDescriptorsPerHeap;

    /// The increment size of a descriptor
    cc::uint32 const mDescriptorHandleIncrementSize;

    /// The descriptor handle cache
    cc::array<D3D12_CPU_DESCRIPTOR_HANDLE> mDescriptorHandleCache;

    /// Descriptor handle cache per descriptor table
    cc::array<descriptor_table_cache, max_descriptor_tables> mDescriptorTableCache;

    /// Bits represents indices in the root signature containing a descriptor table
    cc::uint32 mDescriptorTableBitMask = 0u;

    /// Bits represent indices in the root signature containing a descriptor table that has changed since last copy
    cc::uint32 mStaleDescriptorTableBitMask = 0u;

    cc::vector<ID3D12DescriptorHeap*> mDescriptorHeapPool;
    cc::vector<ID3D12DescriptorHeap*> mAvailableDescriptorHeaps;
    ID3D12DescriptorHeap* mCurrentDescriptorHeap = nullptr;

    CD3DX12_GPU_DESCRIPTOR_HANDLE mCurrentGPUDescriptorHandle = D3D12_DEFAULT;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mCurrentCPUDescriptorHandle = D3D12_DEFAULT;

    int mNumFreeHandles = 0;
};


template <bool compute_mode>
void pr::backend::d3d12::GPUDescriptorPool::commitStagedDescriptors(pr::backend::d3d12::CommandList& commandList)
{
    if (prepareDescriptorCommit(commandList))
    {
        auto& graphics_command_list = commandList.GetGraphicsCommandList();

        DWORD root_index;
        // Scan from LSB to MSB for a bit set in staleDescriptorsBitMask
        while (_BitScanForward(&root_index, mStaleDescriptorTableBitMask))
        {
            auto const& table = mDescriptorTableCache[root_index];

            // Copy the staged CPU visible descriptors to the GPU visible descriptor heap.
            D3D12_CPU_DESCRIPTOR_HANDLE const dest_descriptor_range_starts[] = {mCurrentCPUDescriptorHandle};
            UINT const dest_descriptor_range_sizes[] = {UINT(table.size)};
            mDevice.CopyDescriptors(1, dest_descriptor_range_starts, dest_descriptor_range_sizes, UINT(table.size), table.base_descriptor, nullptr, mDescriptorHeapType);

            // Set the descriptors on the command list
            if constexpr (compute_mode)
                graphics_command_list.SetComputeRootDescriptorTable(root_index, mCurrentGPUDescriptorHandle);
            else
                graphics_command_list.SetGraphicsRootDescriptorTable(root_index, mCurrentGPUDescriptorHandle);

            // Offset current CPU and GPU descriptor handles.
            mCurrentCPUDescriptorHandle.Offset(table.size, mDescriptorHandleIncrementSize);
            mCurrentGPUDescriptorHandle.Offset(table.size, mDescriptorHandleIncrementSize);
            mNumFreeHandles -= table.size;

            // Flip the stale bit so the descriptor table is not recopied again unless it is updated with a new descriptor.
            mStaleDescriptorTableBitMask ^= (1 << root_index);
        }
    }
}
}

#endif
