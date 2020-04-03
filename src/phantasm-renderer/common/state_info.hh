#pragma once

#include <clean-core/array.hh>
#include <clean-core/capped_vector.hh>
#include <clean-core/hash.hh>

#include <phantasm-hardware-interface/arguments.hh>
#include <phantasm-hardware-interface/detail/type_operators.hh>
#include <phantasm-hardware-interface/types.hh>

#include <typed-geometry/types/size.hh>

namespace pr
{
struct pipeline_state_info
{
    phi::pipeline_config graphics_config = {};
    unsigned vertex_size_bytes = 0;
    bool has_root_consts = false;
    cc::capped_vector<phi::vertex_attribute_info, 8> vertex_attributes;
    cc::capped_vector<phi::arg::shader_arg_shape, phi::limits::max_shader_arguments> arg_shapes;
    cc::capped_vector<cc::array<uint64_t, 2>, 5> shader_hashes;
    phi::arg::framebuffer_config framebuffer_config;

    bool operator==(pipeline_state_info const& rhs) const noexcept
    {
        using namespace phi;
        return graphics_config == rhs.graphics_config &&     //
               vertex_size_bytes == rhs.vertex_size_bytes && //
               has_root_consts == rhs.has_root_consts &&     //
               vertex_attributes == rhs.vertex_attributes && //
               arg_shapes == rhs.arg_shapes &&               //
               shader_hashes == rhs.shader_hashes &&         //
               framebuffer_config == rhs.framebuffer_config; //
    }
};

struct state_info_hasher
{
};
}
