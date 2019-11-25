#include <clean-core/assert.hh>

#define STBI_ASSERT(_arg_) CC_ASSERT(_arg_)

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.hh"
