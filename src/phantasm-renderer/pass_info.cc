#include "pass_info.hh"

#include <phantasm-hardware-interface/common/sse_hash.hh>

uint64_t pr::framebuffer_info::get_hash() const { return phi::util::sse_hash_type(&_storage); }
