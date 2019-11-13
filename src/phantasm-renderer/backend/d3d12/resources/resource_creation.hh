#pragma once

#include <clean-core/span.hh>

#include "resource.hh"
#include "resource_view.hh"

namespace pr::backend::d3d12
{
class UploadHeap;

//
// create resources
//

/// return state: COPY_DEST
resource create_texture2d(ResourceAllocator& allocator, int w, int h, DXGI_FORMAT format, int mips, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

/// return state: DEPTH_WRITE
resource create_depth_stencil(ResourceAllocator& allocator, int w, int h, DXGI_FORMAT format);

/// return state: RENDER_TARGET
resource create_render_target(ResourceAllocator& allocator, int w, int h, DXGI_FORMAT format);

/// return state: given initial state
resource create_buffer(ResourceAllocator& allocator, size_t size_bytes, D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_COMMON);

//
// create views (descriptors) on resources
//

void make_rtv(resource const& res, D3D12_CPU_DESCRIPTOR_HANDLE handle, int mip = -1);

void make_srv(resource const& res, D3D12_CPU_DESCRIPTOR_HANDLE handle, int mip = -1);

void make_dsv(resource const& res, D3D12_CPU_DESCRIPTOR_HANDLE handle, int array_slice = 1);

void make_uav(resource const& res, D3D12_CPU_DESCRIPTOR_HANDLE handle);

void make_cube_srv(resource const& res, D3D12_CPU_DESCRIPTOR_HANDLE handle);

//
// create views on buffers
//

[[nodiscard]] D3D12_VERTEX_BUFFER_VIEW make_vertex_buffer_view(resource const& res, size_t vertex_size);

[[nodiscard]] D3D12_INDEX_BUFFER_VIEW make_index_buffer_view(resource const& res, size_t index_size = sizeof(int));

[[nodiscard]] D3D12_CONSTANT_BUFFER_VIEW_DESC make_constant_buffer_view(resource const& res);

//
// create initialized resources from files or data
// variants using an upload buffer only initiate copies, and do not flush
// all resources are created in COPY_DEST state unless noted otherwise
//

resource create_texture2d_from_file(ResourceAllocator& allocator, ID3D12Device& device, UploadHeap& upload_heap, char const* filename, bool use_srgb = false);

resource create_buffer_from_data(ResourceAllocator& allocator, UploadHeap& upload_heap, size_t size, void const* data);

/// must be explicitly templated due to the span deduction issue
template <class ElementT>
resource create_buffer_from_data(ResourceAllocator& allocator, UploadHeap& upload_heap, cc::span<ElementT const> elements)
{
    return create_buffer_from_data(allocator, upload_heap, elements.size() * sizeof(ElementT), elements.data());
}
}
