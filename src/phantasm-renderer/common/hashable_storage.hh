#pragma once

#include <type_traits>

#include <clean-core/new.hh>
#include <clean-core/storage.hh>

#include "murmur_hash.hh"

namespace pr
{
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

    // copy ctor/assign is memcpy to preserve the memset padding - noexcept to allow noexcept default move in outer types
    hashable_storage(hashable_storage const& rhs) noexcept { std::memcpy(&_storage.value, &rhs._storage.value, sizeof(T)); }
    hashable_storage& operator=(hashable_storage const& rhs) noexcept
    {
        if (this != &rhs)
            std::memcpy(&_storage.value, &rhs._storage.value, sizeof(T));
        return *this;
    }

    hashable_storage(hashable_storage&& rhs) noexcept = delete;
    hashable_storage& operator=(hashable_storage&& rhs) noexcept = delete;

    // with the established guarantees, hashing and comparison are trivial
    void get_murmur(murmur_hash& out) const { murmurhash3_x64_128(&_storage.value, sizeof(T), 0, out); }
    bool operator==(hashable_storage<T> const& rhs) const noexcept { return std::memcmp(&_storage.value, &rhs._storage.value, sizeof(T)) == 0; }

private:
    cc::storage_for<T> _storage;
};
}
