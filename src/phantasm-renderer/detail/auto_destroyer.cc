#include "auto_destroyer.hh"

#include <phantasm-hardware-interface/Backend.hh>

#include <phantasm-renderer/Context.hh>

void pr::detail::auto_destroy_proxy::cache_deref(pr::Context* ctx, const pr::buffer& v) { ctx->free_to_cache(v); }
void pr::detail::auto_destroy_proxy::cache_deref(pr::Context* ctx, const pr::texture& v) { ctx->free_to_cache(v); }

bool pr::detail::auto_destroy_proxy::is_destroy_legal(pr::Context* ctx) { return ctx->is_shutting_down(); }

void pr::detail::auto_destroy_proxy::destroy(pr::Context* ctx, const pr::buffer& v) { ctx->free_untyped(v.handle); }
void pr::detail::auto_destroy_proxy::destroy(pr::Context* ctx, const pr::texture& v) { ctx->free_untyped(v.handle); }
void pr::detail::auto_destroy_proxy::destroy(pr::Context* ctx, const pipeline_state_abstract& v) { ctx->freePipelineState(v.handle); }
void pr::detail::auto_destroy_proxy::destroy(pr::Context* ctx, const pr::shader_binary& v) { ctx->free(v); }
void pr::detail::auto_destroy_proxy::destroy(pr::Context* ctx, const pr::prebuilt_argument& v) { ctx->freeShaderView(v._sv); }
void pr::detail::auto_destroy_proxy::destroy(pr::Context* ctx, const pr::fence& v) { ctx->free(v); }

void pr::detail::auto_destroy_proxy::destroy(pr::Context* ctx, const pr::query_range& v) { ctx->free(v); }
void pr::detail::auto_destroy_proxy::destroy(pr::Context* ctx, const pr::swapchain& v) { ctx->free(v); }
