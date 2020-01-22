#pragma once

#include <phantasm-hardware-interface/assets/vertex_attrib_info.hh>

namespace pr
{
template <class VertexT>
class VertexShader
{
public:
    inline static auto const sAttributes = phi::assets::get_vertex_attributes<VertexT>();


};
}
