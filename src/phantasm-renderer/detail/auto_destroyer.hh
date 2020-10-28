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

    static bool is_destroy_legal(pr::Context* ctx);

    static void destroy(pr::Context* ctx, buffer const& v);
    static void destroy(pr::Context* ctx, render_target const& v);
    static void destroy(pr::Context* ctx, texture const& v);
    static void destroy(pr::Context* ctx, pipeline_state_abstract const& v);
    static void destroy(pr::Context* ctx, shader_binary const& v);
    static void destroy(pr::Context* ctx, prebuilt_argument const& v);
    static void destroy(pr::Context* ctx, fence const& v);
    static void destroy(pr::Context* ctx, query_range const& v);
    static void destroy(pr::Context* ctx, swapchain const& v);
};
}

template <class T, auto_mode Mode>
struct auto_destroyer
{
    T data;

    /// disable RAII management and receive the POD contents, which must now be manually freed
    [[nodiscard]] T disown()
    {
        CC_ASSERT(parent != nullptr && "Attempted to disown an invalid auto_ type");
        parent = nullptr;
        return data;
    }

    /// free prematurely (to cache or not), invalid afterwards
    void free()
    {
        if (parent != nullptr)
        {
            if constexpr (Mode == auto_mode::cache)
            {
                detail::auto_destroy_proxy::cache_deref(parent, data);
            }
            else if constexpr (Mode == auto_mode::destroy || Mode == auto_mode::guard)
            {
                // explicit call - also valid in guard mode
                detail::auto_destroy_proxy::destroy(parent, data);
            }

            parent = nullptr;
        }
    }

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

    [[deprecated("renamed to .disown()")]] [[nodiscard]] T unlock() { return disown(); }

    operator T&() & { return data; }
    operator T const&() const& { return data; }

    [[deprecated("auto_ types are move-only")]] auto_destroyer(auto_destroyer const&) = delete;
    [[deprecated("auto_ types are move-only")]] auto_destroyer& operator=(auto_destroyer const&) = delete;

    // force unlock from rvalue
    [[deprecated("discarding auto_ type here would destroy contents, use .disown()")]] operator T&() && = delete;
    [[deprecated("discarding auto_ type here would destroy contents, use .disown()")]] operator T const&() const&& = delete;

private:
    pr::Context* parent = nullptr;

    void _destroy()
    {
        if (parent != nullptr)
        {
            if constexpr (Mode == auto_mode::cache)
            {
                detail::auto_destroy_proxy::cache_deref(parent, data);
            }
            else if constexpr (Mode == auto_mode::destroy)
            {
                detail::auto_destroy_proxy::destroy(parent, data);
            }
            else if constexpr (Mode == auto_mode::guard)
            {
                CC_ASSERT(detail::auto_destroy_proxy::is_destroy_legal(parent) && "discarded auto_ type without .disown() (and before a context shutdown)");
            }

            parent = nullptr;
        }
    }
};
}
