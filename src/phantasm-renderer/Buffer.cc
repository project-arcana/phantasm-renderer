#include "Buffer.hh"

#include "Context.hh"

void pr::on_buffer_free(pr::Context* ctx, phi::handle::resource res, uint64_t guid, const pr::buffer_info& info) { ctx->freeBuffer(info, res, guid); }
