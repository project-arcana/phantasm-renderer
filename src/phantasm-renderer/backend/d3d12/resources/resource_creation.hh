//#pragma once

//#include <clean-core/span.hh>

//#include "resource.hh"
//#include "resource_view.hh"

//namespace pr::backend::d3d12
//{
//class UploadHeap;

////
//// create resources
////
////
//// create views (descriptors) on resources
//// NOTE: these might be superfluous in the face of default descriptor handles
////

//void make_rtv(ID3D12Resource* res, D3D12_CPU_DESCRIPTOR_HANDLE handle, int mip = -1);

//void make_srv(ID3D12Resource* res, D3D12_CPU_DESCRIPTOR_HANDLE handle, int mip = -1);

//void make_dsv(ID3D12Resource* res, D3D12_CPU_DESCRIPTOR_HANDLE handle, int array_slice = 1);

//void make_uav(ID3D12Resource* res, D3D12_CPU_DESCRIPTOR_HANDLE handle);

//void make_cube_srv(ID3D12Resource* res, D3D12_CPU_DESCRIPTOR_HANDLE handle);

////
//// create views on buffers
////

//[[nodiscard]] D3D12_VERTEX_BUFFER_VIEW make_vertex_buffer_view(resource const& res, size_t vertex_size);

//[[nodiscard]] D3D12_INDEX_BUFFER_VIEW make_index_buffer_view(resource const& res, size_t index_size = sizeof(int));

//[[nodiscard]] D3D12_CONSTANT_BUFFER_VIEW_DESC make_constant_buffer_view(resource const& res);

////
//// create initialized resources from files or data
//// variants using an upload buffer only initiate copies, and do not flush
//// all resources are created in COPY_DEST state unless noted otherwise
////

//resource create_texture2d_from_file(ResourceAllocator& allocator, ID3D12Device& device, UploadHeap& upload_heap, char const* filename, bool use_srgb = false);

//resource create_buffer_from_data(ResourceAllocator& allocator, UploadHeap& upload_heap, size_t size, void const* data);

///// must be explicitly templated due to the span deduction issue
//template <class ElementT>
//resource create_buffer_from_data(ResourceAllocator& allocator, UploadHeap& upload_heap, cc::span<ElementT const> elements)
//{
//    return create_buffer_from_data(allocator, upload_heap, elements.size() * sizeof(ElementT), elements.data());
//}
//}
