#pragma once

namespace pr::backend
{
enum class resource_state
{
    // unknown to pr
    unknown,
    // undefined in API semantics
    undefined,

    vertex_buffer,
    index_buffer,

    // D3D12 naming
    constant_buffer,
    shader_resource,
    unordered_access,

    render_target,
    depth_read,
    depth_write,

    // argument for an indirect draw or dispatch
    indirect_argument,

    copy_src,
    copy_dest,

    present,

    raytrace_accel_struct,
};

}
