#include "GraphicsPass.hh"

#include <clean-core/assert.hh>

#include "Frame.hh"

void pr::raii::GraphicsPass::draw(uint32_t num_vertices, uint32_t num_instances)
{
    draw(phi::handle::null_resource, phi::handle::null_resource, num_vertices, num_instances);
}

void pr::raii::GraphicsPass::draw(const pr::buffer& vertex_buffer, uint32_t num_instances, int vertex_offset)
{
    auto const& bufferDesc = mParent->mCtx->get_backend().getResourceBufferDescription(vertex_buffer.handle);
    CC_ASSERT(bufferDesc.stride_bytes > 0 && "vertex buffer not strided");
    draw(vertex_buffer.handle, phi::handle::null_resource, bufferDesc.size_bytes / bufferDesc.stride_bytes, num_instances, vertex_offset, 0u);
}

void pr::raii::GraphicsPass::draw(const pr::buffer& vertex_buffer, const pr::buffer& index_buffer, uint32_t num_instances, int vertex_offset, uint32_t index_offset)
{
    auto const& vertexDesc = mParent->mCtx->get_backend().getResourceBufferDescription(vertex_buffer.handle);
    auto const& indexDesc = mParent->mCtx->get_backend().getResourceBufferDescription(index_buffer.handle);

    CC_ASSERT(vertexDesc.stride_bytes > 0 && "vertex buffer not strided");
    CC_ASSERT(indexDesc.stride_bytes > 0 && "index buffer not strided");
    CC_ASSERT(indexDesc.stride_bytes <= 4 && "index buffer stride unusually large - switched up vertex and index buffer?");
    draw(vertex_buffer.handle, index_buffer.handle, indexDesc.size_bytes / indexDesc.stride_bytes, num_instances, vertex_offset, index_offset);
}

void pr::raii::GraphicsPass::draw(phi::handle::resource vertex_buffer, phi::handle::resource index_buffer, uint32_t num_indices, uint32_t num_instances, int vertex_offset, uint32_t index_offset)
{
    CC_ASSERT(mCmd.pipeline_state.is_valid() && "PSO is invalid at drawcall submission");

    mCmd.vertex_buffers[0] = vertex_buffer;
    mCmd.index_buffer = index_buffer;
    mCmd.num_instances = num_instances;
    mCmd.num_indices = num_indices;
    mCmd.index_offset = index_offset;
    mCmd.vertex_offset = vertex_offset;

    mParent->mBackend->cmdDraw(mParent->mList, mCmd);
}

void pr::raii::GraphicsPass::draw(cc::span<phi::handle::resource const> vertex_buffers,
                                  phi::handle::resource index_buffer,
                                  uint32_t num_indices,
                                  uint32_t num_instances,
                                  int vertex_offset,
                                  uint32_t index_offset)
{
    CC_ASSERT(mCmd.pipeline_state.is_valid() && "PSO is invalid at drawcall submission");
    CC_ASSERT(vertex_buffers.size() <= phi::limits::max_vertex_buffers && "too many vertex buffers supplied");

    mCmd.vertex_buffers[0] = phi::handle::null_resource; // properly invalidate in case span is empty
    std::memcpy(mCmd.vertex_buffers, vertex_buffers.data(), cc::min(sizeof(mCmd.vertex_buffers), vertex_buffers.size_bytes()));
    mCmd.index_buffer = index_buffer;
    mCmd.num_instances = num_instances;
    mCmd.num_indices = num_indices;
    mCmd.index_offset = index_offset;
    mCmd.vertex_offset = vertex_offset;

    mParent->mBackend->cmdDraw(mParent->mList, mCmd);

    // invalidate buffers beyond the first one for subsequent draws
    mCmd.vertex_buffers[1] = phi::handle::null_resource;
}

void raii::GraphicsPass::draw_indirect(phi::handle::resource argument_buffer, phi::handle::resource vertex_buffer, phi::handle::resource index_buffer, uint32_t num_args, uint32_t arg_buffer_offset_bytes)
{
    CC_ASSERT(mCmd.pipeline_state.is_valid() && "PSO is invalid at drawcall submission");

    phi::cmd::draw_indirect dcmd = {};
    std::memcpy(dcmd.root_constants, mCmd.root_constants, sizeof(dcmd.root_constants));
    std::memcpy(dcmd.shader_arguments.data(), mCmd.shader_arguments.data(), sizeof(dcmd.shader_arguments));
    dcmd.pipeline_state = mCmd.pipeline_state;
    dcmd.indirect_argument.buffer = argument_buffer;
    dcmd.indirect_argument.offset_bytes = arg_buffer_offset_bytes;
    dcmd.max_num_arguments = num_args;
    dcmd.vertex_buffers[0] = vertex_buffer;
    dcmd.index_buffer = index_buffer;
    dcmd.argument_type = index_buffer.is_valid() ? phi::indirect_command_type::draw_indexed : phi::indirect_command_type::draw;

    mParent->mBackend->cmdDrawIndirect(mParent->mList, dcmd);
}

void pr::raii::GraphicsPass::draw_indirect(const pr::buffer& argument_buffer, const pr::buffer& vertex_buffer, uint32_t num_args, uint32_t arg_buffer_offset_bytes)
{
    draw_indirect(argument_buffer.handle, vertex_buffer.handle, phi::handle::null_resource, num_args, arg_buffer_offset_bytes);
}

void pr::raii::GraphicsPass::draw_indirect(buffer const& argument_buffer, buffer const& vertex_buffer, buffer const& index_buffer, uint32_t num_args, uint32_t arg_buffer_offset_bytes)
{
    draw_indirect(argument_buffer.handle, vertex_buffer.handle, index_buffer.handle, num_args, arg_buffer_offset_bytes);
}

void pr::raii::GraphicsPass::add_cached_argument(pr::argument& arg, phi::handle::resource cbv, uint32_t cbv_offset)
{
    ++mArgNum;
    mCmd.add_shader_arg(cbv, cbv_offset, mParent->passAcquireGraphicsShaderView(arg));
}
