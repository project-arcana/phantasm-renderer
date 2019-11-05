#pragma once

#include <phantasm-renderer/primitive_pipeline_config.hh>

namespace pr
{
/**
 * This object is implicitly convertible to default configurations for most types
 */
static constexpr struct default_config_t
{
    operator primitive_pipeline_config() const { return {}; }
} default_config;
}
