#pragma once

#include <clean-core/assert.hh>

#include <phantasm-renderer/fwd.hh>

namespace pr
{
namespace detail
{
struct auto_destroy_proxy
{
    static void cache_deref(pr::Context* ctx, buffer const& v);
    static void cache_deref(pr::Context* ctx, render_target const& v);
    static void cache_deref(pr::Context* ctx, texture const& v);
    static void destroy(pr::Context* ctx, buffer const& v);
    static void destroy(pr::Context* ctx, render_target const& v);
    static void destroy(pr::Context* ctx, texture const& v);
    static void destroy(pr::Context* ctx, pipeline_state_abstract const& v);
    static void destroy(pr::Context* ctx, shader_binary const& v);
    static void destroy(pr::Context* ctx, prebuilt_argument const& v);
};
}

template <class T, bool Cache>
struct auto_destroyer
{
    T data;

    auto_destroyer() = default;
    auto_destroyer(T const& data, pr::Context* parent) : data(data), parent(parent) {}

    auto_destroyer(auto_destroyer&& rhs) noexcept : data(rhs.data), parent(rhs.parent) { rhs.parent = nullptr; }
    auto_destroyer& operator=(auto_destroyer&& rhs) noexcept
    {
        if (this != &rhs)
        {
            _destroy();
            data = rhs.data;
            parent = rhs.parent;
            rhs.parent = nullptr;
        }
        return *this;
    }

    ~auto_destroyer() { _destroy(); }

    /// disable RAII management and receive the POD contents, which must now be manually freed
    [[nodiscard]] T const& unlock()
    {
        CC_ASSERT(parent != nullptr && "Attempted to unlock an invalid auto_ type");
        parent = nullptr;
        return data;
    }

    /// free prematurely (to cache or not), invalid afterwards
    void free() { _destroy(); }

    operator T&() & { return data; }
    operator T const&() const& { return data; }

    [[deprecated("auto_ types are move-only")]] auto_destroyer(auto_destroyer const&) = delete;
    [[deprecated("auto_ types are move-only")]] auto_destroyer& operator=(auto_destroyer const&) = delete;

    // force unlock from rvalue
    [[deprecated("discarding auto_ type here would destroy contents, use .unlock()")]] operator T&() && = delete;
    [[deprecated("discarding auto_ type here would destroy contents, use .unlock()")]] operator T const&() const&& = delete;

private:
    pr::Context* parent = nullptr;

    void _destroy()
    {
        if (parent != nullptr)
        {
            if constexpr (Cache)
            {
                detail::auto_destroy_proxy::cache_deref(parent, data);
            }
            else
            {
                detail::auto_destroy_proxy::destroy(parent, data);
            }

            parent = nullptr;
        }
    }
};
}
