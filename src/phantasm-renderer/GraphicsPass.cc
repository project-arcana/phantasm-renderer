#include "GraphicsPass.hh"

#include <clean-core/assert.hh>

#include "Frame.hh"

void pr::raii::GraphicsPass::draw(unsigned num_vertices) { draw(phi::handle::null_resource, phi::handle::null_resource, num_vertices); }

void pr::raii::GraphicsPass::draw(const pr::buffer& vertex_buffer)
{
    CC_ASSERT(vertex_buffer.info.stride_bytes > 0 && "vertex buffer not strided");
    draw(vertex_buffer.res.handle, phi::handle::null_resource, vertex_buffer.info.size_bytes / vertex_buffer.info.stride_bytes);
}

void pr::raii::GraphicsPass::draw(const pr::buffer& vertex_buffer, const pr::buffer& index_buffer)
{
    CC_ASSERT(index_buffer.info.stride_bytes > 0 && "index buffer not strided");
    CC_ASSERT(index_buffer.info.stride_bytes <= 4 && "index buffer stride unusually large - switched up vertex and index buffer?");
    draw(vertex_buffer.res.handle, index_buffer.res.handle, index_buffer.info.size_bytes / index_buffer.info.stride_bytes);
}

void pr::raii::GraphicsPass::draw(phi::handle::resource vertex_buffer, phi::handle::resource index_buffer, unsigned num_indices)
{
    mCmd.vertex_buffer = vertex_buffer;
    mCmd.index_buffer = index_buffer;

    mCmd.num_indices = num_indices;

    mParent->passOnDraw(mCmd);
}

void pr::raii::GraphicsPass::add_argument(const pr::argument& arg)
{
    ++mArgNum;
    mCmd.add_shader_arg(phi::handle::null_resource, 0, mParent->passAcquireGraphicsShaderView(arg));
}


void pr::raii::GraphicsPass::add_argument(const pr::argument& arg, const pr::buffer& constant_buffer, uint32_t constant_buffer_offset)
{
    ++mArgNum;
    mCmd.add_shader_arg(constant_buffer.res.handle, constant_buffer_offset, mParent->passAcquireGraphicsShaderView(arg));
}
