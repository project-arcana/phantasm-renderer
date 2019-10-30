#pragma once

#include <deque>  // TODO: custom
#include <memory> // TODO: cc::shared_ptr

#include <phantasm-renderer/backend/d3d12/common/shared_com_ptr.hh>
#include <phantasm-renderer/backend/d3d12/common/d3d12_fwd.hh>

#include "byte_util.hh"

namespace pr::backend::d3d12
{
/// Paged linear allocator for generic data upload memory
/// Adapted from https://www.3dgep.com/learning-directx-12-3/#UploadBuffer_Class
/// Thread-agnostic, unsynchronized
class UploadBuffer
{
public:
    struct allocation
    {
        void* cpu_ptr;
        D3D12_GPU_VIRTUAL_ADDRESS gpu_ptr;
    };

public:
    /// Page size should fit all allocations of a single command list
    explicit UploadBuffer(ID3D12Device& device, size_t page_size = mem::a_2mb);

    UploadBuffer(UploadBuffer const&) = delete;
    UploadBuffer(UploadBuffer&&) noexcept = delete;
    UploadBuffer& operator=(UploadBuffer const&) = delete;
    UploadBuffer& operator=(UploadBuffer&&) noexcept = delete;

    /// Perform an allocation, size must be <= page size
    [[nodiscard]] allocation alloc(size_t size, size_t align);

    /// Release all allocated memory
    void reset();

    [[nodiscard]] size_t getPageSize() const { return mPageSize; }

private:
    struct page
    {
        explicit page(ID3D12Device& dev, size_t size);

        page(page const&) = delete;
        page(page&&) noexcept = delete;
        page& operator=(page const&) = delete;
        page& operator=(page&&) noexcept = delete;

        ~page();

        [[nodiscard]] allocation alloc(size_t size, size_t align);
        void reset();

        [[nodiscard]] bool can_fit_alloc(size_t size, size_t align) const;

    private:
        shared_com_ptr<ID3D12Resource> _resource;
        void* _cpu_ptr = nullptr;
        D3D12_GPU_VIRTUAL_ADDRESS _gpu_ptr = D3D12_GPU_VIRTUAL_ADDRESS(0);

        size_t const _page_size;
        size_t _offset = 0;
    };
    // TODO: These can be unique (semantically, complicates some things internally)
    // possibly dont use smart pointers at all here
    using shared_page_t = std::shared_ptr<page>;
    using page_pool_t = std::deque<shared_page_t>;

private:
    /// Returns a pooled page, or creates a new one
    [[nodiscard]] shared_page_t requestPage();

private:
    ID3D12Device& mDevice;
    size_t const mPageSize;
    page_pool_t mPagePool;
    page_pool_t mAvailablePages;
    shared_page_t mCurrentPage; // TODO: This can be a raw pointer
};

}
