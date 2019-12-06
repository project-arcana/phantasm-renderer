#pragma once

#include <cstdint>

#include <clean-core/span.hh>
#include <clean-core/string.hh>

#include "types.hh"

namespace pr::backend
{
enum class gpu_vendor : uint8_t
{
    amd,
    intel,
    nvidia,
    imgtec,
    arm,
    qualcomm,
    unknown
};

// opaque, API-specific capability level, more is better
enum class gpu_capabilities : uint8_t
{
    insufficient,
    level_1,
    level_2,
    level_3,
    level_4,
    level_5,
    level_6,
    level_7,
    level_8,
    level_9,
    level_10
};

struct gpu_info
{
    cc::string description;
    unsigned index; ///< an index into an API-specific ordering

    size_t dedicated_video_memory_bytes;
    size_t dedicated_system_memory_bytes;
    size_t shared_system_memory_bytes;

    gpu_vendor vendor;
    gpu_capabilities capabilities;
    bool has_raytracing;
};

[[nodiscard]] gpu_vendor get_gpu_vendor_from_id(unsigned vendor_id);

[[nodiscard]] size_t get_preferred_gpu(cc::span<gpu_info const> candidates, adapter_preference preference, bool verbose = true);
}
