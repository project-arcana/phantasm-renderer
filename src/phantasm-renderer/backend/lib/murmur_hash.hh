#pragma once

#include <cstdint>

namespace pr::backend::hash
{
// UNUSED
void murmurhash3_x64_128(const void* key, int len, uint32_t seed, void* out);
}
