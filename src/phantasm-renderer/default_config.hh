#pragma once

#include <phantasm-hardware-interface/types.hh>

namespace pr
{
/**
 * This object is implicitly convertible to default configurations for most types
 */
static constexpr struct default_config_t
{
    operator phi::pipeline_config() const { return {}; }
} default_config;
}
