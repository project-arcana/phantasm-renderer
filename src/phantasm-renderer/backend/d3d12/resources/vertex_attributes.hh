#pragma once

#include <clean-core/capped_vector.hh>

#include <phantasm-renderer/backend/assets/vertex_attrib_info.hh>

#include <phantasm-renderer/backend/d3d12/common/d3d12_sanitized.hh>
#include <phantasm-renderer/backend/d3d12/common/dxgi_format.hh>


namespace pr::backend::d3d12
{
[[nodiscard]] inline cc::capped_vector<D3D12_INPUT_ELEMENT_DESC, 16> get_input_description(cc::span<assets::vertex_attribute_info const> attrib_info)
{
    cc::capped_vector<D3D12_INPUT_ELEMENT_DESC, 16> res;
    for (auto const& ai : attrib_info)
    {
        D3D12_INPUT_ELEMENT_DESC& elem = res.emplace_back();
        elem.SemanticName = ai.semantic_name;
        elem.AlignedByteOffset = ai.offset;
        elem.Format = util::to_dxgi_format(ai.format);
        elem.SemanticIndex = 0;
        elem.InputSlot = 0;
        elem.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        elem.InstanceDataStepRate = 0;
    }

    return res;
}
}
