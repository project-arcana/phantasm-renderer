#include "auto_destroyer.hh"

#include <phantasm-renderer/Context.hh>

void pr::detail::auto_destroy_proxy::cache_deref(pr::Context* ctx, const pr::buffer& v) { ctx->free_to_cache(v); }

void pr::detail::auto_destroy_proxy::cache_deref(pr::Context* ctx, const pr::render_target& v) { ctx->free_to_cache(v); }

void pr::detail::auto_destroy_proxy::cache_deref(pr::Context* ctx, const pr::texture& v) { ctx->free_to_cache(v); }

void pr::detail::auto_destroy_proxy::destroy(pr::Context* ctx, const pr::buffer& v) { ctx->free(v.res); }

void pr::detail::auto_destroy_proxy::destroy(pr::Context* ctx, const pr::render_target& v) { ctx->free(v.res); }

void pr::detail::auto_destroy_proxy::destroy(pr::Context* ctx, const pr::texture& v) { ctx->free(v.res); }

void pr::detail::auto_destroy_proxy::destroy(pr::Context* ctx, const pipeline_state_abstract& v) { ctx->freePipelineState(v._handle); }

void pr::detail::auto_destroy_proxy::destroy(pr::Context* ctx, const pr::shader_binary& v)
{
    if (v._owning_blob != nullptr)
    {
        ctx->freeShaderBinary(v._owning_blob);
    }
}

void pr::detail::auto_destroy_proxy::destroy(pr::Context* ctx, const pr::prebuilt_argument& v) { ctx->freeShaderView(v._sv); }
