#pragma once

#include <cstdlib>

namespace pr::backend::detail
{
struct unique_buffer
{
    unique_buffer() = default;
    unique_buffer(size_t size) : _ptr(std::malloc(size)) {}

    void allocate(size_t size)
    {
        std::free(_ptr);
        if (size > 0)
            _ptr = std::malloc(size);
    }

    unique_buffer(unique_buffer const&) = delete;
    unique_buffer& operator=(unique_buffer const&) = delete;

    unique_buffer(unique_buffer&& rhs) noexcept
    {
        _ptr = rhs._ptr;
        rhs._ptr = nullptr;
    }
    unique_buffer& operator=(unique_buffer&& rhs) noexcept
    {
        if (this != &rhs)
        {
            std::free(_ptr);
            _ptr = rhs._ptr;
            rhs._ptr = nullptr;
        }

        return *this;
    }

    ~unique_buffer() { std::free(_ptr); }

    [[nodiscard]] void* get() const& { return _ptr; }
    [[nodiscard]] void* get() const&& = delete;

    operator void*() const& { return _ptr; }
    operator void*() const&& = delete;

    [[nodiscard]] bool operator==(unique_buffer const& rhs) const { return _ptr == rhs._ptr; }
    [[nodiscard]] bool operator!=(unique_buffer const& rhs) const { return _ptr != rhs._ptr; }
    [[nodiscard]] bool operator==(void const* rhs) const { return _ptr == rhs; }
    [[nodiscard]] bool operator!=(void const* rhs) const { return _ptr != rhs; }

private:
    void* _ptr = nullptr;
};
}
