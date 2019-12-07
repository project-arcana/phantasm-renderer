#include "gpu_info.hh"

#include <iostream>

namespace
{
constexpr char const* get_preference_literal(pr::backend::adapter_preference pref)
{
    switch (pref)
    {
    case pr::backend::adapter_preference::first:
        return "first";
    case pr::backend::adapter_preference::integrated:
        return "integrated";
    case pr::backend::adapter_preference::highest_vram:
        return "highest vram";
    case pr::backend::adapter_preference::explicit_index:
        return "explicit index";
    case pr::backend::adapter_preference::highest_feature_level:
        return "highest feature level";
    }
}

}

size_t pr::backend::get_preferred_gpu(cc::span<const pr::backend::gpu_info> candidates, pr::backend::adapter_preference preference, bool verbose)
{
    auto const make_choice = [&]() -> size_t {
        if (candidates.empty())
            return candidates.size();

        switch (preference)
        {
        case adapter_preference::integrated:
        {
            for (auto i = 0u; i < candidates.size(); ++i)
            {
                // Note that AMD also manufactures integrated GPUs, this is a heuristic
                if (candidates[i].vendor == gpu_vendor::intel)
                    return i;
            }

            // Fall back to the first adapter
            return 0;
        }
        case adapter_preference::highest_vram:
        {
            auto highest_vram_index = 0u;
            for (auto i = 1u; i < candidates.size(); ++i)
            {
                if (candidates[i].dedicated_video_memory_bytes > candidates[highest_vram_index].dedicated_video_memory_bytes)
                    highest_vram_index = i;
            }

            return highest_vram_index;
        }
        case adapter_preference::highest_feature_level:
        {
            auto highest_capability_index = 0u;
            for (auto i = 1u; i < candidates.size(); ++i)
            {
                if (candidates[i].capabilities > candidates[highest_capability_index].capabilities)
                    highest_capability_index = i;
            }

            return highest_capability_index;
        }
        case adapter_preference::first:
            return 0;
        case adapter_preference::explicit_index:
            return candidates.size();
        }

        return 0;
    };

    auto const choice = make_choice();

    if (verbose)
    {
        std::cout << "[pr][backend] ";
        if (choice < candidates.size())
        {
            auto const& chosen = candidates[choice];
            std::cout << "chose gpu #" << chosen.index << " (" << chosen.description.c_str() << ")";
        }
        else
        {
            std::cout << "failed to choose gpu";
        }
        std::cout << " from " << candidates.size() << " candidate" << (candidates.size() == 1 ? "" : "s") << ", preference: " << get_preference_literal(preference) << std::endl;
    }

    return choice;
}

pr::backend::gpu_vendor pr::backend::get_gpu_vendor_from_id(unsigned vendor_id)
{
    switch (vendor_id)
    {
    case 0x1002:
        return gpu_vendor::amd;
    case 0x8086:
        return gpu_vendor::intel;
    case 0x10DE:
        return gpu_vendor::nvidia;
    case 0x1010:
        return gpu_vendor::imgtec;
    case 0x13B5:
        return gpu_vendor::arm;
    case 0x5143:
        return gpu_vendor::qualcomm;
    default:
        return gpu_vendor::unknown;
    }
}