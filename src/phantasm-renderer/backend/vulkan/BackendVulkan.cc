#include "BackendVulkan.hh"

#include <clean-core/array.hh>
#include <iostream>
#include <phantasm-renderer/backend/device_tentative/window.hh>

#include "common/debug_callback.hh"
#include "common/verify.hh"
#include "common/zero_struct.hh"
#include "gpu_choice_util.hh"
#include "layer_extension_util.hh"
#include "loader/volk.hh"

namespace pr::backend::vk
{
struct BackendVulkan::per_thread_component
{
    //    command_list_translator translator;
    CommandAllocatorBundle cmd_list_allocator;
};
}

void pr::backend::vk::BackendVulkan::initialize(const backend_config& config, device::Window& window)
{
    PR_VK_VERIFY_SUCCESS(volkInitialize());

    VkApplicationInfo app_info;
    zero_info_struct(app_info, VK_STRUCTURE_TYPE_APPLICATION_INFO);
    app_info.pApplicationName = "phantasm-renderer application";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "phantasm-renderer";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_1;

    auto const active_lay_ext = get_used_instance_lay_ext(get_available_instance_lay_ext(), config);

    VkInstanceCreateInfo instance_info;
    zero_info_struct(instance_info, VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledExtensionCount = uint32_t(active_lay_ext.extensions.size());
    instance_info.ppEnabledExtensionNames = active_lay_ext.extensions.empty() ? nullptr : active_lay_ext.extensions.data();
    instance_info.enabledLayerCount = uint32_t(active_lay_ext.layers.size());
    instance_info.ppEnabledLayerNames = active_lay_ext.layers.empty() ? nullptr : active_lay_ext.layers.data();

    // Create the instance
    VkResult create_res = vkCreateInstance(&instance_info, nullptr, &mInstance);

    // TODO: More fine-grained error handling
    PR_VK_ASSERT_SUCCESS(create_res);

    // Load Vulkan entrypoints (instance-based)
    // NOTE: volk is up to 7% slower if using this method (over i.e. volkLoadDevice(VkDevice))
    // We could possibly fastpath somehow for single-device use, or use volkLoadDeviceTable
    // See https://github.com/zeux/volk#optimizing-device-calls
    volkLoadInstance(mInstance);

    if (config.validation != validation_level::off)
    {
        // Debug callback
        createDebugMessenger();
    }

    window.createVulkanSurface(mInstance, mSurface);

    // TODO: Adapter choice
    auto const gpus = get_physical_devices(mInstance);
    for (auto const gpu : gpus)
    {
        auto const info = get_gpu_information(gpu, mSurface);

        if (info.is_suitable)
        {
            mDevice.initialize(info, mSurface, config);

            mAllocator.initialize(mDevice.getPhysicalDevice(), mDevice.getDevice());

            mSwapchain.initialize(mDevice, mSurface, config.num_backbuffers, window.getWidth(), window.getHeight(), config.present_mode);

            break;
        }
    }

    // Pool init
    mPoolPipelines.initialize(mDevice.getDevice(), config.max_num_pipeline_states);
    mPoolResources.initialize(mDevice.getPhysicalDevice(), mDevice.getDevice(), config.max_num_resources);
    mPoolShaderViews.initialize(mDevice.getDevice(), &mPoolResources, config.max_num_shader_view_elements / 2, config.max_num_shader_view_elements / 2,
                                config.max_num_shader_view_elements / 2, config.max_num_shader_view_elements / 2); // NOTE: slightly arbitrary mapping

    // Per-thread components and command list pool
    {
        mThreadAssociation.initialize();
        mThreadComponents = mThreadComponents.defaulted(config.num_threads);

        cc::vector<CommandAllocatorBundle*> thread_allocator_ptrs;
        thread_allocator_ptrs.reserve(config.num_threads);

        for (auto& thread_comp : mThreadComponents)
        {
            //            thread_comp.translator.initialize(&device, &mPoolShaderViews, &mPoolResources, &mPoolPSOs);
            thread_allocator_ptrs.push_back(&thread_comp.cmd_list_allocator);
        }

        mPoolCmdLists.initialize(*this, config.num_cmdlist_allocators_per_thread, config.num_cmdlists_per_allocator, thread_allocator_ptrs);
    }
}

pr::backend::vk::BackendVulkan::~BackendVulkan()
{
    flushGPU();
    mSwapchain.destroy();

    mPoolShaderViews.destroy();
    mPoolCmdLists.destroy();
    mPoolPipelines.destroy();
    mPoolResources.destroy();

    for (auto& thread_cmp : mThreadComponents)
    {
        thread_cmp.cmd_list_allocator.destroy(mDevice.getDevice());
    }

    vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
    mAllocator.destroy();
    mDevice.destroy();

    if (mDebugMessenger != VK_NULL_HANDLE)
        vkDestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);

    vkDestroyInstance(mInstance, nullptr);
}

pr::backend::handle::command_list pr::backend::vk::BackendVulkan::recordCommandList(std::byte* buffer, size_t size)
{
    auto& thread_comp = mThreadComponents[mThreadAssociation.get_current_index()];

    VkCommandBuffer raw_list;
    auto const res = mPoolCmdLists.create(raw_list, thread_comp.cmd_list_allocator);
    //    thread_comp.translator.translateCommandList(raw_list, mPoolCmdLists.getStateCache(res), buffer, size);
    return res;
}

void pr::backend::vk::BackendVulkan::submit(cc::span<const pr::backend::handle::command_list> cls)
{
    constexpr auto c_batch_size = 16;

    cc::capped_vector<VkCommandBuffer, c_batch_size * 2> submit_batch;
    //    cc::capped_vector<handle::command_list, c_batch_size> barrier_lists;
    unsigned last_cl_index = 0;
    unsigned num_cls_in_batch = 0;

    // TODO
    VkPipelineStageFlags const submitWaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    auto& thread_comp = mThreadComponents[mThreadAssociation.get_current_index()];

    auto const submit_flush = [&]() {
        VkFence submit_fence;
        auto const submit_fence_index = mPoolCmdLists.acquireFence(submit_fence);

        VkSubmitInfo submit_info;
        zero_info_struct(submit_info, VK_STRUCTURE_TYPE_SUBMIT_INFO);
        submit_info.pWaitDstStageMask = &submitWaitStage;
        submit_info.commandBufferCount = unsigned(submit_batch.size());
        submit_info.pCommandBuffers = submit_batch.data();

        PR_VK_VERIFY_SUCCESS(vkQueueSubmit(mDevice.getQueueGraphics(), 1, &submit_info, submit_fence));

        //        mPoolCmdLists.freeOnSubmit(barrier_lists, submit_fence_index);
        mPoolCmdLists.freeOnSubmit(cls.subspan(last_cl_index, num_cls_in_batch), submit_fence_index);

        submit_batch.clear();
        //        barrier_lists.clear();
        last_cl_index += num_cls_in_batch;
        num_cls_in_batch = 0;
    };

    for (auto const cl : cls)
    {
        if (cl == handle::null_command_list)
            continue;

        //        auto const* const state_cache = mPoolCmdLists.getStateCache(cl);
        //        cc::capped_vector<D3D12_RESOURCE_BARRIER, 32> barriers;

        //        for (auto const& entry : state_cache->cache)
        //        {
        //            auto const master_before = mPoolResources.getResourceState(entry.ptr);

        //            if (master_before != entry.required_initial)
        //            {
        //                // transition to the state required as the initial one
        //                auto& barrier = barriers.emplace_back();
        //                util::populate_barrier_desc(barrier, mPoolResources.getRawResource(entry.ptr), util::to_native(master_before),
        //                                            util::to_native(entry.required_initial));
        //            }

        //            // set the master state to the one in which this resource is left
        //            mPoolResources.setResourceState(entry.ptr, entry.current);
        //        }

        //        if (!barriers.empty())
        //        {
        //            ID3D12GraphicsCommandList* t_cmd_list;
        //            barrier_lists.push_back(mPoolCmdLists.create(t_cmd_list, thread_comp.cmd_list_allocator));
        //            t_cmd_list->ResourceBarrier(UINT(barriers.size()), barriers.size() > 0 ? barriers.data() : nullptr);
        //            t_cmd_list->Close();
        //            submit_batch.push_back(t_cmd_list);
        //        }

        submit_batch.push_back(mPoolCmdLists.getRawBuffer(cl));
        ++num_cls_in_batch;

        if (num_cls_in_batch == c_batch_size)
            submit_flush();
    }

    if (num_cls_in_batch > 0)
        submit_flush();
}

void pr::backend::vk::BackendVulkan::flushGPU() { vkDeviceWaitIdle(mDevice.getDevice()); }

void pr::backend::vk::BackendVulkan::createDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    zero_info_struct(createInfo, VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT);
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                 | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = detail::debug_callback;
    createInfo.pUserData = nullptr;
    PR_VK_VERIFY_SUCCESS(vkCreateDebugUtilsMessengerEXT(mInstance, &createInfo, nullptr, &mDebugMessenger));
}
