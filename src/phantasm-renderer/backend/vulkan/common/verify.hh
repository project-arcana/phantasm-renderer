#pragma once
#ifdef PR_BACKEND_VULKAN

#include <clean-core/macros.hh>

#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

namespace pr::backend::vk::detail
{
[[noreturn]] CC_COLD_FUNC CC_DONT_INLINE void vk_verify_failure_handler(VkResult vr, char const* expression, char const* filename, int line);
}

// TODO: option to disable verify in release builds
// NOTE: possibly merge this somehow with CC_ASSERT

/// Executes the given expression and terminates with a detailed error message if the VkResult indicates failure
#define PR_VK_VERIFY(_expr_)                                                                           \
    do                                                                                                 \
    {                                                                                                  \
        ::VkResult const op_res = (_expr_);                                                            \
        if (CC_UNLIKELY(op_res != VK_SUCCESS))                                                         \
        {                                                                                              \
            ::pr::backend::vk::detail::vk_verify_failure_handler(op_res, #_expr_, __FILE__, __LINE__); \
        }                                                                                              \
    } while (0)
#endif
