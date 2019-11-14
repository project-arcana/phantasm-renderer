#pragma once

#include <clean-core/vector.hh>
#include <clean-core/array.hh>

#include "loader/volk.hh"

namespace pr::backend::vk
{
struct suitable_queues
{
    cc::vector<int> indices_graphics;
    cc::vector<int> indices_compute;
    cc::vector<int> indices_copy;
};

// graphics, compute, copy
using chosen_queues = cc::array<int, 3>;

[[nodiscard]] suitable_queues get_suitable_queues(VkPhysicalDevice physical, VkSurfaceKHR surface);

[[nodiscard]] chosen_queues get_chosen_queues(suitable_queues const& suitable);

}
