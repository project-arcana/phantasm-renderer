#pragma once

#include <phantasm-hardware-interface/arguments.hh>

#include <phantasm-renderer/reflection/vertex_attributes.hh>

namespace pr
{
template <class VertexT>
class VertexShader
{
public:
    inline static auto const sAttributes = get_vertex_attributes<VertexT>();
    phi::arg::shader_binary binary;
};
}
