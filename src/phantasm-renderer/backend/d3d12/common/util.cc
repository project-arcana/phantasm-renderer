#include "util.hh"

#include <corecrt_wstdio.h>

#include <phantasm-renderer/backend/d3d12/common/dxgi_format.hh>

cc::capped_vector<D3D12_INPUT_ELEMENT_DESC, 16> pr::backend::d3d12::util::get_native_vertex_format(cc::span<const pr::backend::vertex_attribute_info> attrib_info)
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

void pr::backend::d3d12::util::set_object_name(ID3D12Object* object, const char* name)
{
    if (name != nullptr)
    {
        wchar_t name_wide[1024];
        ::swprintf(name_wide, 1024, L"%S", name);
        object->SetName(name_wide);
    }
}
