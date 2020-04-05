#pragma once

#include <cstdint>
#include <cstring>

#include <clean-core/hash_combine.hh>

namespace pr
{
struct murmur_hash
{
    uint64_t value[2];

    constexpr bool operator==(murmur_hash const& rhs) const noexcept { return value[0] == rhs.value[0] && value[1] == rhs.value[1]; }

    [[nodiscard]] murmur_hash combine(murmur_hash rhs) const noexcept
    {
        return {{cc::hash_combine(value[0], rhs.value[0]), cc::hash_combine(value[1], rhs.value[1])}};
    }
};

struct murmur_collapser
{
    constexpr size_t operator()(murmur_hash v) const noexcept { return cc::hash_combine(v.value[0], v.value[1]); }
};

/**
 * @brief murmurhash3_x64_128
 * @param key - Data to hash
 * @param len - Size of the data in bytes
 * @param seed
 * @param out - pointer write the 128 bit result to, i.e. uint64_t[2]
 */
void murmurhash3_x64_128(const void* key, int len, uint32_t seed, murmur_hash& out);



}
