#include "adapter_choice_util.hh"
#ifdef PR_BACKEND_D3D12

#include <array>

#include <clean-core/assert.hh>

#include "common/safe_seh_call.hh"
#include "common/shared_com_ptr.hh"
#include "common/verify.hh"


pr::backend::d3d12::adapter_vendor pr::backend::d3d12::get_vendor_from_id(unsigned id)
{
    switch (id)
    {
    case 0x1002:
        return adapter_vendor::amd;
    case 0x8086:
        return adapter_vendor::intel;
    case 0x10DE:
        return adapter_vendor::nvidia;
    default:
        return adapter_vendor::unknown;
    }
}

int pr::backend::d3d12::test_adapter(IDXGIAdapter* adapter, D3D_FEATURE_LEVEL min_features, D3D_FEATURE_LEVEL& out_max_features)
{
    std::array const all_feature_levels = {D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0};

    int res_nodes = -1;

    detail::perform_safe_seh_call([&] {
        shared_com_ptr<ID3D12Device> test_device;
        auto const hres = ::D3D12CreateDevice(adapter, min_features, PR_COM_WRITE(test_device));

        if (SUCCEEDED(hres))
        {
            D3D12_FEATURE_DATA_FEATURE_LEVELS feature_data;
            feature_data.pFeatureLevelsRequested = all_feature_levels.data();
            feature_data.NumFeatureLevels = unsigned(all_feature_levels.size());

            if (SUCCEEDED(test_device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &feature_data, sizeof(feature_data))))
            {
                out_max_features = feature_data.MaxSupportedFeatureLevel;
            }
            else
            {
                out_max_features = min_features;
            }

            res_nodes = int(test_device->GetNodeCount());
            CC_ASSERT(res_nodes >= 0);
        }
    });

    return res_nodes;
}

std::vector<pr::backend::d3d12::adapter_candidate> pr::backend::d3d12::get_adapter_candidates()
{
    auto constexpr min_candidate_feature_level = D3D_FEATURE_LEVEL_12_0;

    // Create a temporary factory to enumerate adapters
    shared_com_ptr<IDXGIFactory4> temp_factory;
    detail::perform_safe_seh_call([&] { PR_D3D12_VERIFY(::CreateDXGIFactory(PR_COM_WRITE(temp_factory))); });

    // If the call failed (likely XP or earlier), return empty
    if (!temp_factory.is_valid())
        return {};

    std::vector<adapter_candidate> res;

    shared_com_ptr<IDXGIAdapter> temp_adapter;
    for (uint32_t i = 0u; temp_factory->EnumAdapters(i, temp_adapter.override()) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        if (temp_adapter.is_valid())
        {
            D3D_FEATURE_LEVEL max_feature_level = D3D_FEATURE_LEVEL(0);
            auto num_nodes = test_adapter(temp_adapter, min_candidate_feature_level, max_feature_level);

            if (num_nodes >= 0)
            {
                // Min level supported, this adapter is a candidate
                DXGI_ADAPTER_DESC adapter_desc;
                PR_D3D12_VERIFY(temp_adapter->GetDesc(&adapter_desc));

                std::wstring description_wide = adapter_desc.Description;

                auto& new_candidate = res.emplace_back();
                new_candidate.vendor = get_vendor_from_id(adapter_desc.VendorId);
                new_candidate.index = i;

                new_candidate.dedicated_video_memory_bytes = adapter_desc.DedicatedVideoMemory;
                new_candidate.dedicated_system_memory_bytes = adapter_desc.DedicatedSystemMemory;
                new_candidate.shared_system_memory_bytes = adapter_desc.SharedSystemMemory;

                new_candidate.description = std::string(description_wide.begin(), description_wide.end());
                new_candidate.max_feature_level = max_feature_level;
            }
        }
    }

    return res;
}

#endif

uint32_t pr::backend::d3d12::get_preferred_adapter_index(const std::vector<pr::backend::d3d12::adapter_candidate>& candidates,
                                                         pr::backend::d3d12::adapter_preference preference)
{
    if (candidates.empty())
        return uint32_t(-1);

    switch (preference)
    {
    case adapter_preference::integrated:
    {
        for (auto const& cand : candidates)
        {
            // Note that AMD also manufactures integrated GPUs, this is a heuristic
            if (cand.vendor == adapter_vendor::intel)
                return cand.index;
        }

        // Fall back to the first adapter
        return candidates[0].index;
    }
    case adapter_preference::highest_vram:
    {
        auto highest_vram_index = 0u;
        for (auto i = 1u; i < candidates.size(); ++i)
        {
            if (candidates[i].dedicated_video_memory_bytes > candidates[highest_vram_index].dedicated_video_memory_bytes)
                highest_vram_index = i;
        }

        return candidates[highest_vram_index].index;
    }
    case adapter_preference::highest_feature_level:
    {
        auto highest_feature_index = 0u;
        for (auto i = 1u; i < candidates.size(); ++i)
        {
            if (candidates[i].max_feature_level > candidates[highest_feature_index].max_feature_level)
                highest_feature_index = i;
        }

        return candidates[highest_feature_index].index;
    }
    case adapter_preference::first:
        return candidates[0].index;
    }

    return candidates[0].index;
}
