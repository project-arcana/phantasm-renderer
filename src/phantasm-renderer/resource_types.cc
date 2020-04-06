#include "resource_types.hh"

#include "Context.hh"

void pr::shader_binary_pod::destroy(pr::Context* ctx)
{
    if (_owning_blob != nullptr)
    {
        ctx->freeShaderBinary(_owning_blob);
    }
}

// void pr::buffer::unrefCache(pr::Context* ctx) { ctx->freeCachedBuffer(_info, cc::move(_resource)); }

// void pr::render_target::unrefCache(pr::Context* ctx) { ctx->freeCachedTarget(_info, cc::move(_resource)); }

void pr::pipeline_state_abstract::destroy(pr::Context* ctx) { ctx->freePipelineState(_handle); }

void pr::buffer::cache_deref(pr::Context* parent) { parent->freeCachedBuffer(info, res); }

void pr::buffer::destroy(pr::Context* parent) { parent->freeResource(res.handle); }

void pr::render_target::cache_deref(pr::Context* parent) { parent->freeCachedTarget(info, res); }

void pr::render_target::destroy(pr::Context* parent) { parent->freeResource(res.handle); }

void pr::texture::cache_deref(pr::Context* parent) { parent->freeCachedTexture(info, res); }

void pr::texture::destroy(pr::Context* parent) { parent->freeResource(res.handle); }
