#include "GraphicsPass.hh"

#include <clean-core/assert.hh>

#include "Frame.hh"

void pr::GraphicsPass::draw(unsigned num_vertices)
{
    mCmd.vertex_buffer = phi::handle::null_resource;
    mCmd.index_buffer = phi::handle::null_resource;

    mCmd.num_indices = num_vertices;

    mCmd.vertex_offset = 0;
    mCmd.index_offset = 0;
    mCmd.scissor = tg::iaabb2(-1, -1);

    mParent->pipelineOnDraw(mCmd);
}

void pr::GraphicsPass::draw(const pr::buffer& vertex_buffer)
{
    CC_ASSERT(vertex_buffer._info.stride_bytes > 0 && "vertex buffer not strided");

    mCmd.vertex_buffer = vertex_buffer._resource.data.handle;
    mCmd.index_buffer = phi::handle::null_resource;

    mCmd.num_indices = vertex_buffer._info.size_bytes / vertex_buffer._info.stride_bytes;

    mCmd.vertex_offset = 0;
    mCmd.index_offset = 0;
    mCmd.scissor = tg::iaabb2(-1, -1);

    mParent->pipelineOnDraw(mCmd);
}

void pr::GraphicsPass::draw(const pr::buffer& vertex_buffer, const pr::buffer& index_buffer)
{
    CC_ASSERT(index_buffer._info.stride_bytes > 0 && "index buffer not strided");

    mCmd.vertex_buffer = vertex_buffer._resource.data.handle;
    mCmd.index_buffer = index_buffer._resource.data.handle;

    mCmd.num_indices = index_buffer._info.size_bytes / index_buffer._info.stride_bytes;

    mCmd.vertex_offset = 0;
    mCmd.index_offset = 0;
    mCmd.scissor = tg::iaabb2(-1, -1);

    mParent->pipelineOnDraw(mCmd);
}

void pr::GraphicsPass::set_constant_buffer(const pr::buffer& constant_buffer, unsigned offset)
{
    CC_ASSERT(mArgNum != 0 && "Attempted to set_constant_buffer on a GraphicsPass without prior bind");
    mCmd.shader_arguments[uint8_t(mArgNum - 1)].constant_buffer = constant_buffer._resource.data.handle;
    mCmd.shader_arguments[uint8_t(mArgNum - 1)].constant_buffer_offset = offset;
}

void pr::GraphicsPass::set_constant_buffer_offset(unsigned offset)
{
    CC_ASSERT(mArgNum != 0 && "Attempted to set_constant_buffer_offset on a GraphicsPass without prior bind");
    mCmd.shader_arguments[uint8_t(mArgNum - 1)].constant_buffer_offset = offset;
}

void pr::GraphicsPass::add_argument(const pr::baked_argument& sv)
{
    ++mArgNum;
    mCmd.add_shader_arg(phi::handle::null_resource, 0, sv.data._sv);
}

void pr::GraphicsPass::add_argument(const pr::baked_argument& sv, const pr::buffer& constant_buffer, uint32_t constant_buffer_offset)
{
    ++mArgNum;
    mCmd.add_shader_arg(constant_buffer._resource.data.handle, constant_buffer_offset, sv.data._sv);
}

void pr::GraphicsPass::add_argument(const pr::buffer& constant_buffer, uint32_t constant_buffer_offset)
{
    ++mArgNum;
    mCmd.add_shader_arg(constant_buffer._resource.data.handle, constant_buffer_offset);
}
