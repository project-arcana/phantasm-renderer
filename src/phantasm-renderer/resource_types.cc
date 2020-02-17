#include "resource_types.hh"

#include "Context.hh"

void pr::resource_data::destroy(pr::Context* ctx) { ctx->freeResource(handle); }

void pr::shader_binary_data::destroy(pr::Context* ctx)
{
    if (_owning_blob != nullptr)
    {
        ctx->freeShaderBinary(_owning_blob);
    }
}

void pr::buffer::unrefCache(pr::Context* ctx) { ctx->freeCachedBuffer(_info, cc::move(_resource)); }

void pr::render_target::unrefCache(pr::Context* ctx) { ctx->freeCachedTarget(_info, cc::move(_resource)); }

void pr::shader_binary_unreffable::unrefCache(pr::Context* ctx)
{
    // TODO
    CC_RUNTIME_ASSERT(false && "unimplemented");
}

void pr::pipeline_state_abstract::destroy(pr::Context* ctx) { ctx->freePipelineState(_handle); }
