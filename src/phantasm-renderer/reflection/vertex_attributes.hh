#pragma once

#include <cstdint>

#include <clean-core/capped_vector.hh>

#include <reflector/members.hh>

#include <typed-geometry/detail/comp_traits.hh>

#include <phantasm-hardware-interface/types.hh>

namespace pr
{
/// Returns a capped vector of vertex attribute infos
template <class VertT>
[[nodiscard]] auto get_vertex_attributes();


namespace detail
{
template <class T>
constexpr phi::format to_attribute_format()
{
    using af = phi::format;

    if constexpr (tg::is_comp_like<T, 4, float>)
        return af::rgba32f;
    else if constexpr (tg::is_comp_like<T, 3, float>)
        return af::rgb32f;
    else if constexpr (tg::is_comp_like<T, 2, float>)
        return af::rg32f;
    else if constexpr (std::is_same_v<T, float>)
        return af::r32f;

    else if constexpr (tg::is_comp_like<T, 4, int32_t>)
        return af::rgba32i;
    else if constexpr (tg::is_comp_like<T, 3, int32_t>)
        return af::rgb32i;
    else if constexpr (tg::is_comp_like<T, 2, int32_t>)
        return af::rg32i;
    else if constexpr (std::is_same_v<T, int32_t>)
        return af::r32i;

    else if constexpr (tg::is_comp_like<T, 4, uint32_t>)
        return af::rgba32u;
    else if constexpr (tg::is_comp_like<T, 3, uint32_t>)
        return af::rgb32u;
    else if constexpr (tg::is_comp_like<T, 2, uint32_t>)
        return af::rg32u;
    else if constexpr (std::is_same_v<T, uint32_t>)
        return af::r32u;

    else if constexpr (tg::is_comp_like<T, 4, int16_t>)
        return af::rgba16i;
    else if constexpr (tg::is_comp_like<T, 2, int16_t>)
        return af::rg16i;
    else if constexpr (std::is_same_v<T, int16_t>)
        return af::r16i;

    else if constexpr (tg::is_comp_like<T, 4, uint16_t>)
        return af::rgba16u;
    else if constexpr (tg::is_comp_like<T, 2, uint16_t>)
        return af::rg16u;
    else if constexpr (std::is_same_v<T, uint16_t>)
        return af::r16u;

    else if constexpr (tg::is_comp_like<T, 4, int8_t>)
        return af::rgba8i;
    else if constexpr (tg::is_comp_like<T, 2, int8_t>)
        return af::rg8i;
    else if constexpr (std::is_same_v<T, int8_t>)
        return af::r8i;

    else if constexpr (tg::is_comp_like<T, 4, uint8_t>)
        return af::rgba8u;
    else if constexpr (tg::is_comp_like<T, 2, uint8_t>)
        return af::rg8u;
    else if constexpr (std::is_same_v<T, uint8_t>)
        return af::r8u;

    else
    {
        static_assert(sizeof(T) == 0, "incompatible attribute type");
        return af::rgba32f;
    }
}

template <class T>
static constexpr phi::format as_attribute_format = detail::to_attribute_format<T>();

template <class VertT>
struct vertex_visitor
{
    static constexpr auto num_attributes = rf::member_count<VertT>;
    cc::capped_vector<phi::vertex_attribute_info, num_attributes> attributes;

    template <class T>
    void operator()(T const& ref, char const* name)
    {
        phi::vertex_attribute_info& attr = attributes.emplace_back();
        attr.semantic_name = name;
        // This call assumes that the original VertT& in get_vertex_attributes is a dereferenced nullptr
        attr.offset = uint32_t(reinterpret_cast<size_t>(&ref));
        attr.fmt = as_attribute_format<T>;
    }
};
}


template <class VertT>
[[nodiscard]] auto get_vertex_attributes()
{
    static_assert(rf::is_introspectable<VertT>, "Vertex type must be introspectable for automatic attribute extraction."
                                                "Provide `template <class In> constexpr void introspect(In&& insp, Vertex& vert);'");
    detail::vertex_visitor<VertT> visitor;
    VertT* volatile dummy_ptr = nullptr;
    rf::do_introspect(visitor, *dummy_ptr);
    return visitor.attributes;
}

}
