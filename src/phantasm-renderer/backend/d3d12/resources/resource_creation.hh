#pragma once

#include "resource.hh"
#include "resource_view.hh"

namespace pr::backend::d3d12
{
class UploadHeap;

//
// create resources
//
resource create_texture2d(ResourceAllocator& allocator, int w, int h, DXGI_FORMAT format, int mips, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

resource create_depth_stencil(ResourceAllocator& allocator, int w, int h, DXGI_FORMAT format);

resource create_render_target(ResourceAllocator& allocator, int w, int h, DXGI_FORMAT format);

//
// create views (descriptors) on resources
//
void make_rtv(resource const& res, resource_view& rtv, unsigned index, int mip = -1);

void make_srv(resource const& res, resource_view& srv, unsigned index, int mip = -1);

void make_dsv(resource const& res, resource_view& dsv, unsigned index, int array_slice = 1);

void make_uav(resource const& res, resource_view& uav, unsigned index);

void make_cube_srv(resource const& res, resource_view& srv, unsigned index);

//
// create initialized resources from files or data
//
resource create_texture2d_from_file(ResourceAllocator& allocator, ID3D12Device& device, UploadHeap& upload_heap, char const* filename, bool use_srgb = false);
}
