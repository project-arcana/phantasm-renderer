#include "GraphicsPass.hh"

#include <clean-core/assert.hh>

#include "Frame.hh"

pr::GraphicsPass::GraphicsPass(pr::Frame* parent, phi::handle::pipeline_state pso) : mParent(parent), mPSO(pso) { mDrawCmd.pipeline_state = mPSO; }

void pr::GraphicsPass::draw(unsigned num_vertices)
{
    mDrawCmd.vertex_buffer = phi::handle::null_resource;
    mDrawCmd.index_buffer = phi::handle::null_resource;

    mDrawCmd.num_indices = num_vertices;

    mDrawCmd.vertex_offset = 0;
    mDrawCmd.index_offset = 0;
    mDrawCmd.scissor = tg::iaabb2(-1, -1);

    mParent->pipelineOnDraw(mDrawCmd);
}

void pr::GraphicsPass::draw(const pr::buffer& vertex_buffer)
{
    CC_ASSERT(vertex_buffer._info.stride_bytes > 0 && "vertex buffer not strided");

    mDrawCmd.vertex_buffer = vertex_buffer._resource.data.handle;
    mDrawCmd.index_buffer = phi::handle::null_resource;

    mDrawCmd.num_indices = vertex_buffer._info.size_bytes / vertex_buffer._info.stride_bytes;

    mDrawCmd.vertex_offset = 0;
    mDrawCmd.index_offset = 0;
    mDrawCmd.scissor = tg::iaabb2(-1, -1);

    mParent->pipelineOnDraw(mDrawCmd);
}

void pr::GraphicsPass::draw_indexed(const pr::buffer& vertex_buffer, const pr::buffer& index_buffer)
{
    CC_ASSERT(index_buffer._info.stride_bytes > 0 && "index buffer not strided");

    mDrawCmd.vertex_buffer = vertex_buffer._resource.data.handle;
    mDrawCmd.index_buffer = index_buffer._resource.data.handle;

    mDrawCmd.num_indices = index_buffer._info.size_bytes / index_buffer._info.stride_bytes;

    mDrawCmd.vertex_offset = 0;
    mDrawCmd.index_offset = 0;
    mDrawCmd.scissor = tg::iaabb2(-1, -1);

    mParent->pipelineOnDraw(mDrawCmd);
}

void pr::GraphicsPass::add_argument(const pr::baked_argument& sv) { mDrawCmd.add_shader_arg(phi::handle::null_resource, 0, sv.data._sv); }

void pr::GraphicsPass::add_argument(const pr::baked_argument& sv, const pr::buffer& constant_buffer, uint32_t constant_buffer_offset)
{
    mDrawCmd.add_shader_arg(constant_buffer._resource.data.handle, constant_buffer_offset, sv.data._sv);
}

void pr::GraphicsPass::add_argument(const pr::buffer& constant_buffer, uint32_t constant_buffer_offset)
{
    mDrawCmd.add_shader_arg(constant_buffer._resource.data.handle, constant_buffer_offset);
}

void pr::GraphicsPass::pop_argument() { mDrawCmd.shader_arguments.pop_back(); }

void pr::GraphicsPass::clear_arguments() { mDrawCmd.shader_arguments.clear(); }
