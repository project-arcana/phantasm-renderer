#pragma once

#include <cstdint>
#include <cstring>

#include <type_traits>

#include <clean-core/new.hh>
#include <clean-core/storage.hh>

namespace pr
{
struct murmur_hash
{
    uint64_t value[2];

    constexpr bool operator==(murmur_hash const& rhs) const noexcept { return value[0] == rhs.value[0] && value[1] == rhs.value[1]; }
};

struct murmur_collapser
{
    constexpr size_t operator()(murmur_hash v) const noexcept { return v.value[0] ^ v.value[1]; }
};

/**
 * @brief murmurhash3_x64_128
 * @param key - Data to hash
 * @param len - Size of the data in bytes
 * @param seed
 * @param out - pointer write the 128 bit result to, i.e. uint64_t[2]
 */
void murmurhash3_x64_128(const void* key, int len, uint32_t seed, murmur_hash& out);

template <class T>
struct hashable_storage
{
    static_assert(std::is_trivially_destructible_v<T>, "T must not have a destructor");
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");

public:
    T& get() { return _storage.value; }
    T const& get() const { return _storage.value; }


    hashable_storage()
    {
        std::memset(&_storage.value, 0, sizeof(T)); // memset padding and unitialized bytes to 0
        new (&_storage.value) T();                  // call default ctor
    }

    // copy ctor/assign is memcpy to preserve the memset padding
    hashable_storage(hashable_storage const& rhs) { std::memcpy(&_storage.value, &rhs._storage.value, sizeof(T)); }
    hashable_storage& operator=(hashable_storage const& rhs) { std::memcpy(&_storage.value, &rhs._storage.value, sizeof(T)); }

    hashable_storage(hashable_storage&&) = delete;
    hashable_storage& operator=(hashable_storage&&) = delete;

    // with the established guarantees, hashing and comparison are trivial
    void get_murmur(murmur_hash& out) const { murmurhash3_x64_128(&_storage.value, sizeof(T), 0, out); }
    bool operator==(hashable_storage<T> const& rhs) const noexcept { return std::memcmp(&_storage.value, &rhs._storage.value, sizeof(T)) == 0; }

private:
    cc::storage_for<T> _storage;
};

}
