#pragma once

#include <phantasm-renderer/backend/assets/vertex_attrib_info.hh>

namespace pr
{
template <class VertexT>
class VertexShader
{
public:
    inline static auto const sAttributes = backend::assets::get_vertex_attributes<VertexT>();


};
}
