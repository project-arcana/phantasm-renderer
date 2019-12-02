#include "cmd_buf_translation.hh"

#include <phantasm-renderer/backend/detail/byte_util.hh>
#include <phantasm-renderer/backend/detail/incomplete_state_cache.hh>

#include "Swapchain.hh"
#include "common/native_enum.hh"
#include "common/util.hh"
#include "common/verify.hh"
#include "pools/cmd_list_pool.hh"
#include "pools/pipeline_layout_cache.hh"
#include "pools/pipeline_pool.hh"
#include "pools/resource_pool.hh"
#include "pools/shader_view_pool.hh"

void pr::backend::vk::command_list_translator::translateCommandList(
    VkCommandBuffer list, handle::command_list list_handle, pr::backend::detail::incomplete_state_cache* state_cache, std::byte* buffer, size_t buffer_size)
{
    _cmd_list = list;
    _cmd_list_handle = list_handle;
    _state_cache = state_cache;

    _bound.reset();
    _state_cache->reset();

    // auto const gpu_heaps = _globals.pool_shader_views->getGPURelevantHeaps();
    //_cmd_list->SetDescriptorHeaps(UINT(gpu_heaps.size()), gpu_heaps.data());

    // translate all contained commands
    command_stream_parser parser(buffer, buffer_size);
    for (auto const& cmd : parser)
        cmd::detail::dynamic_dispatch(cmd, *this);

    // close the list
    PR_VK_VERIFY_SUCCESS(vkEndCommandBuffer(_cmd_list));

    // done
}

void pr::backend::vk::command_list_translator::execute(const pr::backend::cmd::begin_render_pass& begin_rp)
{
    // We can't call vkCmdBeginRenderPass or anything yet
    _bound.current_render_pass = begin_rp;
    util::set_viewport(_cmd_list, begin_rp.viewport.x, begin_rp.viewport.y);
}

void pr::backend::vk::command_list_translator::execute(const pr::backend::cmd::draw& draw)
{
    if (draw.pipeline_state != _bound.pipeline_state)
    {
        // a new handle::pipeline_state invalidates (!= always changes)
        //      - The bound render pass
        //      - The bound pipeline layout
        //      - The bound pipeline

        _bound.pipeline_state = draw.pipeline_state;
        auto const& pso_node = _globals.pool_pipeline_states->get(draw.pipeline_state);

        if (pso_node.associated_pipeline_layout->raw_layout != _bound.raw_pipeline_layout)
        {
            // a new layout is used when binding arguments,
            // and invalidates previously bound arguments
            _bound.set_pipeline_layout(pso_node.associated_pipeline_layout->raw_layout);
        }

        if (pso_node.associated_render_pass != _bound.raw_render_pass)
        {
            if (_bound.raw_render_pass != nullptr)
            {
                vkCmdEndRenderPass(_cmd_list);
            }

            // a render pass always changes
            //      - The framebuffer
            //      - The vkCmdBeginRenderPass/vkCmdEndRenderPass state
            _bound.raw_render_pass = pso_node.associated_render_pass;

            // create a new framebuffer
            {
                cc::capped_vector<VkImageView, 9> attachments;
                for (auto const& rt : _bound.current_render_pass.render_targets)
                {

                }

                if (_bound.current_render_pass.depth_target.view_info.resource != handle::null_resource)
                {

                }

                VkFramebufferCreateInfo fb_info = {};
                fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                fb_info.pNext = nullptr;
                fb_info.renderPass = pso_node.associated_render_pass;
                fb_info.attachmentCount = static_cast<unsigned>(attachments.size());
                fb_info.pAttachments = attachments.data();
                fb_info.width = static_cast<unsigned>(_bound.current_render_pass.viewport.x);
                fb_info.height = static_cast<unsigned>(_bound.current_render_pass.viewport.y);
                fb_info.layers = 1;
                PR_VK_VERIFY_SUCCESS(vkCreateFramebuffer(_globals.device, &fb_info, nullptr, &_bound.raw_framebuffer));
                _globals.pool_cmd_lists->addAssociatedFramebuffer(_cmd_list_handle, _bound.raw_framebuffer);
            }
        }
    }
}

void pr::backend::vk::command_list_translator::execute(const pr::backend::cmd::end_render_pass&)
{
    // do nothing
}
