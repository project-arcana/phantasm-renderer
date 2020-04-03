#pragma once

#include <cstdint>

namespace pr
{
/**
 * @brief murmurhash3_x64_128
 * @param key - Data to hash
 * @param len - Size of the data in bytes
 * @param seed
 * @param out - pointer write the 128 bit result to, i.e. uint64_t[2]
 */
void murmurhash3_x64_128(const void* key, int len, uint32_t seed, void* out);
}
