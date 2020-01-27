#include "Image.hh"

#include "Context.hh"

void pr::detail::on_image_free(pr::Context* ctx, uint64_t guid, phi::handle::resource res, const pr::texture_info& tex_info)
{
    ctx->freeTexture(tex_info, res, guid);
}

void pr::detail::on_image_free(pr::Context* ctx, uint64_t guid, phi::handle::resource res, const pr::render_target_info& rt_info)
{
    ctx->freeRenderTarget(rt_info, res, guid);
}
