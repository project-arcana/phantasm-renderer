#include "GraphicsPass.hh"

#include <clean-core/assert.hh>

#include "Frame.hh"

void pr::raii::GraphicsPass::draw(uint32_t num_vertices, uint32_t num_instances)
{
    draw(phi::handle::null_resource, phi::handle::null_resource, num_vertices, num_instances);
}

void pr::raii::GraphicsPass::draw(const pr::buffer& vertex_buffer, uint32_t num_instances)
{
    CC_ASSERT(vertex_buffer.info.stride_bytes > 0 && "vertex buffer not strided");
    draw(vertex_buffer.res.handle, phi::handle::null_resource, vertex_buffer.info.size_bytes / vertex_buffer.info.stride_bytes, num_instances);
}

void pr::raii::GraphicsPass::draw(const pr::buffer& vertex_buffer, const pr::buffer& index_buffer, uint32_t num_instances)
{
    CC_ASSERT(vertex_buffer.info.stride_bytes > 0 && "vertex buffer not strided");
    CC_ASSERT(index_buffer.info.stride_bytes > 0 && "index buffer not strided");
    CC_ASSERT(index_buffer.info.stride_bytes <= 4 && "index buffer stride unusually large - switched up vertex and index buffer?");
    draw(vertex_buffer.res.handle, index_buffer.res.handle, index_buffer.info.size_bytes / index_buffer.info.stride_bytes, num_instances);
}

void pr::raii::GraphicsPass::draw(phi::handle::resource vertex_buffer, phi::handle::resource index_buffer, uint32_t num_indices, uint32_t num_instances)
{
    CC_ASSERT(mCmd.pipeline_state.is_valid() && "PSO is invalid at drawcall submission");

    mCmd.vertex_buffer = vertex_buffer;
    mCmd.index_buffer = index_buffer;
    mCmd.num_instances = num_instances;
    mCmd.num_indices = num_indices;

    mParent->passOnDraw(mCmd);
}

void raii::GraphicsPass::draw_indirect(phi::handle::resource argument_buffer, phi::handle::resource vertex_buffer, phi::handle::resource index_buffer, uint32_t num_args, uint32_t arg_buffer_offset_bytes)
{
    CC_ASSERT(mCmd.pipeline_state.is_valid() && "PSO is invalid at drawcall submission");

    phi::cmd::draw_indirect dcmd;
    std::memcpy(dcmd.root_constants, mCmd.root_constants, sizeof(dcmd.root_constants));
    std::memcpy(dcmd.shader_arguments.data(), mCmd.shader_arguments.data(), sizeof(dcmd.shader_arguments));
    dcmd.pipeline_state = mCmd.pipeline_state;
    dcmd.indirect_argument_buffer = argument_buffer;
    dcmd.argument_buffer_offset_bytes = arg_buffer_offset_bytes;
    dcmd.num_arguments = num_args;
    dcmd.vertex_buffer = vertex_buffer;
    dcmd.index_buffer = index_buffer;

    mParent->write_raw_cmd(dcmd);
}

void pr::raii::GraphicsPass::draw_indirect(const pr::buffer& argument_buffer, const pr::buffer& vertex_buffer, uint32_t num_args, uint32_t arg_buffer_offset_bytes)
{
    draw_indirect(argument_buffer.res.handle, vertex_buffer.res.handle, phi::handle::null_resource, num_args, arg_buffer_offset_bytes);
}

void pr::raii::GraphicsPass::draw_indirect(buffer const& argument_buffer, buffer const& vertex_buffer, buffer const& index_buffer, uint32_t num_args, uint32_t arg_buffer_offset_bytes)
{
    draw_indirect(argument_buffer.res.handle, vertex_buffer.res.handle, index_buffer.res.handle, num_args, arg_buffer_offset_bytes);
}

void pr::raii::GraphicsPass::add_cached_argument(const pr::argument& arg, phi::handle::resource cbv, uint32_t cbv_offset)
{
    ++mArgNum;
    mCmd.add_shader_arg(cbv, cbv_offset, mParent->passAcquireGraphicsShaderView(arg));
}
