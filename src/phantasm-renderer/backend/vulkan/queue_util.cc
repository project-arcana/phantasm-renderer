#include "queue_util.hh"

pr::backend::vk::suitable_queues pr::backend::vk::get_suitable_queues(VkPhysicalDevice physical)
{
    uint32_t num_families;
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &num_families, nullptr);
    cc::vector<VkQueueFamilyProperties> queue_families(num_families);
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &num_families, queue_families.data());

    suitable_queues res;
    for (auto i = 0u; i < num_families; ++i)
    {
        auto const& family = queue_families[i];
        if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            res.indices_graphics.push_back(int(i));

        if (family.queueFlags & VK_QUEUE_COMPUTE_BIT)
            res.indices_compute.push_back(int(i));

        if (family.queueFlags & VK_QUEUE_TRANSFER_BIT)
            res.indices_copy.push_back(int(i));
    }

    return res;
}

pr::backend::vk::chosen_queues pr::backend::vk::get_chosen_queues(const pr::backend::vk::suitable_queues& suitable)
{
    chosen_queues res;
    cc::fill(res, -1);

    if (!suitable.indices_graphics.empty())
        res[0] = suitable.indices_graphics.front();

    for (auto const compute : suitable.indices_compute)
        if (compute != res[0])
        {
            res[1] = compute;
            break;
        }

    for (auto const copy : suitable.indices_copy)
        if (copy != res[0] && copy != res[1])
        {
            res[2] = copy;
            break;
        }

    return res;
}
