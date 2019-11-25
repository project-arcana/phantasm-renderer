#include "debug_callback.hh"

#include <iostream>

VkBool32 pr::backend::vk::detail::debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                 VkDebugUtilsMessageTypeFlagsEXT type,
                                                 const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                                                 void* user_data)
{
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        std::cerr << "[pr][vk][validation] " << callback_data->pMessage << std::endl;
    }
    return VK_FALSE;
}
