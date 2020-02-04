#pragma once

namespace pr
{
// derive-tag, a bundle of resources
// structs deriving this must be introspectable
// member mapping:
//      image view -> SRV texture by default, UAV texture if tagged
//      buffer (handle/view?) -> StructuredBuffer (SRV), StructuredBufferRW (UAV) if tagged
//      container of raw data -> StructuredBuffer (SRV), uploaded automatically
//      raw data -> merged into a single automatically uploaded CBV
struct resources
{
};
}
