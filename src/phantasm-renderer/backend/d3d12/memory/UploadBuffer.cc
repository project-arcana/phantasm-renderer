#include "UploadBuffer.hh"
#ifdef PR_BACKEND_D3D12

#include <clean-core/allocate.hh>
#include <clean-core/assert.hh>
#include <clean-core/typedefs.hh>

#include <phantasm-renderer/backend/d3d12/common/d3dx12.hh>
#include <phantasm-renderer/backend/d3d12/common/verify.hh>

pr::backend::d3d12::UploadBuffer::UploadBuffer(ID3D12Device& device, size_t page_size) : mDevice(device), mPageSize(page_size) {}

pr::backend::d3d12::UploadBuffer::~UploadBuffer()
{
    for (auto const& ptr : mPagePool)
        cc::free<page>(ptr);
}

pr::backend::d3d12::UploadBuffer::allocation pr::backend::d3d12::UploadBuffer::alloc(size_t size, size_t align)
{
    CC_ASSERT(size <= mPageSize);

    if (mCurrentPage == nullptr || !mCurrentPage->can_fit_alloc(size, align))
        mCurrentPage = acquirePage();

    return mCurrentPage->alloc(size, align);
}

void pr::backend::d3d12::UploadBuffer::reset()
{
    // Clear the current page pointer
    mCurrentPage = nullptr;

    // Make the entire pool available
    mAvailablePages = mPagePool;

    // Reset all available pages
    for (auto const& page : mAvailablePages)
        page->reset();
}

pr::backend::d3d12::UploadBuffer::page* pr::backend::d3d12::UploadBuffer::acquirePage()
{
    if (!mAvailablePages.empty())
    {
        // Return available pages
        auto const res = mAvailablePages.back();
        mAvailablePages.pop_back();
        return res;
    }
    else
    {
        mPagePool.push_back(cc::alloc<page>(mDevice, mPageSize));
        return mPagePool.back();
    }
}


pr::backend::d3d12::UploadBuffer::page::page(ID3D12Device& dev, size_t size) : _page_size(size)
{
    auto const heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto const res_desc = CD3DX12_RESOURCE_DESC::Buffer(_page_size);
    PR_D3D12_VERIFY(dev.CreateCommittedResource(&heap_props,                       //
                                                D3D12_HEAP_FLAG_NONE,              //
                                                &res_desc,                         //
                                                D3D12_RESOURCE_STATE_GENERIC_READ, //
                                                nullptr,                           //
                                                PR_COM_WRITE(_resource)            //
                                                ));

    _gpu_ptr = _resource->GetGPUVirtualAddress();
    _resource->Map(0, nullptr, &_cpu_ptr);
}

pr::backend::d3d12::UploadBuffer::page::~page() { _resource->Unmap(0, nullptr); }

bool pr::backend::d3d12::UploadBuffer::page::can_fit_alloc(size_t size, size_t align) const
{
    auto const size_aligned = mem::align_up(size, align);
    auto const offset_aligned = mem::align_up(_offset, align);
    return offset_aligned + size_aligned <= _page_size;
}

pr::backend::d3d12::UploadBuffer::allocation pr::backend::d3d12::UploadBuffer::page::alloc(size_t size, size_t align)
{
    CC_RUNTIME_ASSERT(can_fit_alloc(size, align));

    auto const size_aligned = mem::align_up(size, align);
    _offset = mem::align_up(_offset, align);

    allocation res;
    res.cpu_ptr = static_cast<cc::byte*>(_cpu_ptr) + _offset;
    res.gpu_ptr = _gpu_ptr + _offset;

    _offset += size_aligned;
    return res;
}

void pr::backend::d3d12::UploadBuffer::page::reset() { _offset = 0; }


#endif
