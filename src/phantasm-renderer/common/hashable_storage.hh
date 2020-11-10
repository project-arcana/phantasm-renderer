#pragma once

#include <type_traits>

#include <clean-core/new.hh>
#include <clean-core/xxHash.hh>

#include "murmur_hash.hh"

namespace pr
{
template <class T>
struct hashable_storage
{
    static_assert(std::is_trivially_destructible_v<T>, "T must not have a destructor");
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
    static_assert(sizeof(T) > 0, "T must be fully known");

public:
    T& get() { return *reinterpret_cast<T*>(_storage); }
    T const& get() const { return *reinterpret_cast<T const*>(_storage); }

    hashable_storage()
    {
        std::memset(_storage, 0, sizeof(_storage)); // memset padding and unitialized bytes to 0

        if constexpr (!std::is_trivially_constructible_v<T, void>)
            new (cc::placement_new, &_storage) T(); // call default ctor
    }

    // with the established guarantees, hashing and comparison are trivial
    void get_murmur(murmur_hash& out) const { murmurhash3_x64_128(_storage, sizeof(T), 31, out); }

    cc::hash_t get_xxhash() const { return cc::hash_xxh3(cc::as_byte_span(_storage), 31); }

    bool operator==(hashable_storage<T> const& rhs) const noexcept { return std::memcmp(_storage.value, rhs._storage, sizeof(_storage)) == 0; }

private:
    alignas(alignof(T)) std::byte _storage[sizeof(T)];
};
}
