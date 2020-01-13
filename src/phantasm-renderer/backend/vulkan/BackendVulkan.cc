#include "BackendVulkan.hh"

#include <iostream>

#include <clean-core/array.hh>

#include <phantasm-renderer/backend/device_tentative/window.hh>

#include "cmd_buf_translation.hh"
#include "common/debug_callback.hh"
#include "common/verify.hh"
#include "common/vk_format.hh"
#include "gpu_choice_util.hh"
#include "layer_extension_util.hh"
#include "loader/volk.hh"
#include "resources/transition_barrier.hh"

namespace
{
constexpr VkPipelineStageFlags gc_submit_wait_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
}

namespace pr::backend::vk
{
struct BackendVulkan::per_thread_component
{
    command_list_translator translator;
    CommandAllocatorBundle cmd_list_allocator;
};
}

void pr::backend::vk::BackendVulkan::initialize(const backend_config& config_arg, device::Window& window)
{
    PR_VK_VERIFY_SUCCESS(volkInitialize());

    // copy explicitly for modifications
    backend_config config = config_arg;

    mDiagnostics.init();
    if (mDiagnostics.is_renderdoc_present() && config.validation >= validation_level::on)
    {
        std::cout << "[pr][backend][vk] info: Validation layers requested while running RenderDoc, disabling due to known crashes" << std::endl;
        config.validation = validation_level::off;
    }

    auto const active_lay_ext = get_used_instance_lay_ext(get_available_instance_lay_ext(), config);

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "phantasm-renderer application";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "phantasm-renderer";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_1;


    VkInstanceCreateInfo instance_info = {};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledExtensionCount = uint32_t(active_lay_ext.extensions.size());
    instance_info.ppEnabledExtensionNames = active_lay_ext.extensions.empty() ? nullptr : active_lay_ext.extensions.data();
    instance_info.enabledLayerCount = uint32_t(active_lay_ext.layers.size());
    instance_info.ppEnabledLayerNames = active_lay_ext.layers.empty() ? nullptr : active_lay_ext.layers.data();

#ifdef VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT
    cc::array<VkValidationFeatureEnableEXT, 3> extended_validation_enables
        = {VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT, VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
           VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT};
#else
    cc::array<VkValidationFeatureEnableEXT, 2> extended_validation_enables
        = {VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT, VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT};
#endif

    VkValidationFeaturesEXT extended_validation_features = {};

    if (config.validation >= validation_level::on_extended)
    {
        // enable GPU-assisted validation
        extended_validation_features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
        extended_validation_features.enabledValidationFeatureCount = static_cast<uint32_t>(extended_validation_enables.size());
        extended_validation_features.pEnabledValidationFeatures = extended_validation_enables.data();

        instance_info.pNext = &extended_validation_features;
    }

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

    // GPU choice and device init
    {
        auto const vk_gpu_infos = get_all_vulkan_gpu_infos(mInstance, mSurface);
        auto const gpu_infos = get_available_gpus(vk_gpu_infos);
        auto const chosen_index = get_preferred_gpu(gpu_infos, config.adapter_preference);
        CC_RUNTIME_ASSERT(chosen_index != gpu_infos.size());

        auto const& chosen_gpu = gpu_infos[chosen_index];
        auto const& chosen_vk_gpu = vk_gpu_infos[chosen_gpu.index];

        mDevice.initialize(chosen_vk_gpu, mSurface, config);
        mSwapchain.initialize(mDevice, mSurface, config.num_backbuffers, window.getWidth(), window.getHeight(), config.present_mode);
    }

    // Pool init
    mPoolPipelines.initialize(mDevice.getDevice(), config.max_num_pipeline_states);
    mPoolResources.initialize(mDevice.getPhysicalDevice(), mDevice.getDevice(), config.max_num_resources);
    mPoolShaderViews.initialize(mDevice.getDevice(), &mPoolResources, config.max_num_cbvs, config.max_num_srvs, config.max_num_uavs, config.max_num_samplers);

    // Per-thread components and command list pool
    {
        mThreadAssociation.initialize();
        mThreadComponents = mThreadComponents.defaulted(config.num_threads);

        cc::vector<CommandAllocatorBundle*> thread_allocator_ptrs;
        thread_allocator_ptrs.reserve(config.num_threads);

        for (auto& thread_comp : mThreadComponents)
        {
            thread_comp.translator.initialize(mDevice.getDevice(), &mPoolShaderViews, &mPoolResources, &mPoolPipelines, &mPoolCmdLists);
            thread_allocator_ptrs.push_back(&thread_comp.cmd_list_allocator);
        }

        mPoolCmdLists.initialize(*this, config.num_cmdlist_allocators_per_thread, config.num_cmdlists_per_allocator, thread_allocator_ptrs);
    }
}

pr::backend::vk::BackendVulkan::~BackendVulkan()
{
    if (mInstance != nullptr)
    {
        flushGPU();

        mDiagnostics.free();

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
        mDevice.destroy();

        if (mDebugMessenger != nullptr)
            vkDestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);

        vkDestroyInstance(mInstance, nullptr);
    }
}

pr::backend::handle::resource pr::backend::vk::BackendVulkan::acquireBackbuffer()
{
    auto const prev_backbuffer_index = mSwapchain.getCurrentBackbufferIndex();
    bool const acquire_success = mSwapchain.waitForBackbuffer();

    if (!acquire_success)
    {
        onInternalResize();
        return handle::null_resource;
    }
    else
    {
        resource_state prev_state;
        auto const res = mPoolResources.injectBackbufferResource(mSwapchain.getCurrentBackbuffer(), mSwapchain.getCurrentBackbufferState(),
                                                                 mSwapchain.getCurrentBackbufferView(), prev_state);

        mSwapchain.setBackbufferState(prev_backbuffer_index, prev_state);
        return res;
    }
}

void pr::backend::vk::BackendVulkan::present()
{
    mSwapchain.performPresentSubmit();
    if (!mSwapchain.present())
    {
        onInternalResize();
    }
}

void pr::backend::vk::BackendVulkan::onResize(tg::isize2 size)
{
    flushGPU();
    onInternalResize();
    mSwapchain.onResize(size.width, size.height);
}

pr::backend::format pr::backend::vk::BackendVulkan::getBackbufferFormat() const { return util::to_pr_format(mSwapchain.getBackbufferFormat()); }

pr::backend::handle::command_list pr::backend::vk::BackendVulkan::recordCommandList(std::byte* buffer, size_t size)
{
    auto& thread_comp = mThreadComponents[mThreadAssociation.get_current_index()];

    VkCommandBuffer raw_list;
    auto const res = mPoolCmdLists.create(raw_list, thread_comp.cmd_list_allocator);
    thread_comp.translator.translateCommandList(raw_list, res, mPoolCmdLists.getStateCache(res), buffer, size);
    return res;
}

void pr::backend::vk::BackendVulkan::submit(cc::span<const pr::backend::handle::command_list> cls)
{
    constexpr auto c_batch_size = 16;

    cc::capped_vector<VkCommandBuffer, c_batch_size * 2> submit_batch;
    cc::capped_vector<handle::command_list, c_batch_size> barrier_lists;
    unsigned last_cl_index = 0;
    unsigned num_cls_in_batch = 0;

    auto& thread_comp = mThreadComponents[mThreadAssociation.get_current_index()];

    auto const submit_flush = [&]() {
        VkFence submit_fence;
        auto const submit_fence_index = mPoolCmdLists.acquireFence(submit_fence);

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pWaitDstStageMask = &gc_submit_wait_stage;
        submit_info.commandBufferCount = unsigned(submit_batch.size());
        submit_info.pCommandBuffers = submit_batch.data();

        PR_VK_VERIFY_SUCCESS(vkQueueSubmit(mDevice.getQueueDirect(), 1, &submit_info, submit_fence));

        cc::array<cc::span<handle::command_list const>, 2> submit_spans = {barrier_lists, cls.subspan(last_cl_index, num_cls_in_batch)};
        mPoolCmdLists.freeOnSubmit(submit_spans, submit_fence_index);

        submit_batch.clear();
        barrier_lists.clear();
        last_cl_index += num_cls_in_batch;
        num_cls_in_batch = 0;
    };

    for (auto const cl : cls)
    {
        if (cl == handle::null_command_list)
            continue;

        auto const* const state_cache = mPoolCmdLists.getStateCache(cl);
        barrier_bundle<32, 32, 32> barriers;

        for (auto const& entry : state_cache->cache)
        {
            auto const master_before = mPoolResources.getResourceState(entry.ptr);

            if (master_before != entry.required_initial)
            {
                auto const master_dep_before = mPoolResources.getResourceStageDependency(entry.ptr);

                // transition to the state required as the initial one
                state_change const change = state_change(master_before, entry.required_initial, master_dep_before, entry.initial_dependency);

                if (mPoolResources.isImage(entry.ptr))
                {
                    auto const& img_info = mPoolResources.getImageInfo(entry.ptr);
                    barriers.add_image_barrier(img_info.raw_image, change, util::to_native_image_aspect(img_info.pixel_format));
                }
                else
                {
                    auto const& buf_info = mPoolResources.getBufferInfo(entry.ptr);
                    barriers.add_buffer_barrier(buf_info.raw_buffer, change, buf_info.width);
                }
            }

            // set the master state to the one in which this resource is left
            mPoolResources.setResourceState(entry.ptr, entry.current, entry.current_dependency);
        }

        if (!barriers.empty())
        {
            VkCommandBuffer t_cmd_list;
            barrier_lists.push_back(mPoolCmdLists.create(t_cmd_list, thread_comp.cmd_list_allocator));
            barriers.record(t_cmd_list);
            vkEndCommandBuffer(t_cmd_list);
            submit_batch.push_back(t_cmd_list);
        }

        submit_batch.push_back(mPoolCmdLists.getRawBuffer(cl));
        ++num_cls_in_batch;

        if (num_cls_in_batch == c_batch_size)
            submit_flush();
    }

    if (num_cls_in_batch > 0)
        submit_flush();
}

void pr::backend::vk::BackendVulkan::printInformation(pr::backend::handle::resource res) const
{
    std::cout << "[pr][backend][vk] Inspecting resource " << res.index << std::endl;
    if (!res.is_valid())
        std::cout << "  invalid (== handle::null_resource)" << std::endl;
    else
    {
        if (mPoolResources.isImage(res))
        {
            auto const& info = mPoolResources.getImageInfo(res);
            std::cout << "[pr][backend][vk]  image, raw pointer: " << info.raw_image << std::endl;
            std::cout << "[pr][backend][vk]  " << info.num_mips << " mips, " << info.num_array_layers
                      << " array layers, format: " << unsigned(info.pixel_format) << std::endl;
        }
        else
        {
            auto const& info = mPoolResources.getBufferInfo(res);
            std::cout << "[pr][backend][vk]  buffer, raw pointer: " << info.raw_buffer << std::endl;
            std::cout << "[pr][backend][vk]  " << info.width << " width, " << info.stride << " stride, raw mapped ptr: " << info.map << std::endl;
            std::cout << "[pr][backend][vk]  raw dynamic CBV descriptor set: " << info.raw_uniform_dynamic_ds << std::endl;
        }
    }
}

bool pr::backend::vk::BackendVulkan::startForcedDiagnosticCapture() { return mDiagnostics.start_capture(); }

bool pr::backend::vk::BackendVulkan::endForcedDiagnosticCapture() { return mDiagnostics.end_capture(); }

void pr::backend::vk::BackendVulkan::flushGPU() { vkDeviceWaitIdle(mDevice.getDevice()); }

void pr::backend::vk::BackendVulkan::createDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                 | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = detail::debug_callback;
    createInfo.pUserData = nullptr;
    PR_VK_VERIFY_SUCCESS(vkCreateDebugUtilsMessengerEXT(mInstance, &createInfo, nullptr, &mDebugMessenger));
}
