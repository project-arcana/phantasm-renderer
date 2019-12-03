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
#include "resources/resource_state.hh"

void pr::backend::vk::command_list_translator::translateCommandList(
    VkCommandBuffer list, handle::command_list list_handle, vk_incomplete_state_cache* state_cache, std::byte* buffer, size_t buffer_size)
{
    _cmd_list = list;
    _cmd_list_handle = list_handle;
    _state_cache = state_cache;

    _bound.reset();
    _state_cache->reset();

    // translate all contained commands
    command_stream_parser parser(buffer, buffer_size);
    for (auto const& cmd : parser)
        cmd::detail::dynamic_dispatch(cmd, *this);

    if (_bound.raw_render_pass != nullptr)
    {
        // end the last render pass
        vkCmdEndRenderPass(_cmd_list);
    }

    // close the list
    PR_VK_VERIFY_SUCCESS(vkEndCommandBuffer(_cmd_list));

    // done
}

void pr::backend::vk::command_list_translator::execute(const pr::backend::cmd::begin_render_pass& begin_rp)
{
    // We can't call vkCmdBeginRenderPass or anything yet
    _bound.current_render_pass = begin_rp;
}

void pr::backend::vk::command_list_translator::execute(const pr::backend::cmd::draw& draw)
{
    if (draw.pipeline_state != _bound.pipeline_state)
    {
        // a new handle::pipeline_state invalidates (!= always changes)
        //      - The bound pipeline layout
        //      - The bound render pass
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
                cc::capped_vector<VkImageView, limits::max_render_targets + 1> attachments;

                for (auto const& rt : _bound.current_render_pass.render_targets)
                {
                    attachments.push_back(_globals.pool_shader_views->makeImageView(rt.sve));
                }

                if (_bound.current_render_pass.depth_target.sve.resource != handle::null_resource)
                {
                    attachments.push_back(_globals.pool_shader_views->makeImageView(_bound.current_render_pass.depth_target.sve));
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

                // Create the framebuffer
                PR_VK_VERIFY_SUCCESS(vkCreateFramebuffer(_globals.device, &fb_info, nullptr, &_bound.raw_framebuffer));

                // Associate it with the current command list so it will get cleaned up
                _globals.pool_cmd_lists->addAssociatedFramebuffer(_cmd_list_handle, _bound.raw_framebuffer);

                // discard image views
                for (auto const imv : attachments)
                {
                    vkDestroyImageView(_globals.device, imv, nullptr);
                }
            }

            // begin a new render pass
            {
                VkRenderPassBeginInfo rp_begin_info = {};
                rp_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                rp_begin_info.renderPass = pso_node.associated_render_pass;
                rp_begin_info.framebuffer = _bound.raw_framebuffer;
                rp_begin_info.renderArea.offset = {0, 0};
                rp_begin_info.renderArea.extent.width = static_cast<uint32_t>(_bound.current_render_pass.viewport.x);
                rp_begin_info.renderArea.extent.height = static_cast<uint32_t>(_bound.current_render_pass.viewport.y);

                cc::capped_vector<VkClearValue, limits::max_render_targets + 1> clear_values;

                for (auto const& rt : _bound.current_render_pass.render_targets)
                {
                    auto& cv = clear_values.emplace_back();
                    std::memcpy(&cv.color.float32, &rt.clear_value, sizeof(rt.clear_value));
                }

                if (_bound.current_render_pass.depth_target.sve.resource != handle::null_resource)
                {
                    auto& cv = clear_values.emplace_back();
                    cv.depthStencil = {_bound.current_render_pass.depth_target.clear_value_depth,
                                       static_cast<uint32_t>(_bound.current_render_pass.depth_target.clear_value_stencil)};
                }

                rp_begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
                rp_begin_info.pClearValues = clear_values.data();

                util::set_viewport(_cmd_list, _bound.current_render_pass.viewport.x, _bound.current_render_pass.viewport.y);
                vkCmdBeginRenderPass(_cmd_list, &rp_begin_info, VK_SUBPASS_CONTENTS_INLINE);
            }
        }

        vkCmdBindPipeline(_cmd_list, VK_PIPELINE_BIND_POINT_GRAPHICS, pso_node.raw_pipeline);
    }

    // Index buffer (optional)
    if (draw.index_buffer != _bound.index_buffer)
    {
        _bound.index_buffer = draw.index_buffer;
        if (draw.index_buffer.is_valid())
        {
            vkCmdBindIndexBuffer(_cmd_list, _globals.pool_resources->getRawBuffer(draw.index_buffer), 0, VK_INDEX_TYPE_UINT32);
        }
        else
        {
            // TODO: can you un-bind index buffers in vulkan? (vkCmdBindIndexBuffer does not take nullptr)
        }
    }

    // Vertex buffer
    if (draw.vertex_buffer != _bound.vertex_buffer)
    {
        _bound.vertex_buffer = draw.vertex_buffer;
        VkBuffer vertex_buffers[] = {_globals.pool_resources->getRawBuffer(draw.vertex_buffer)};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(_cmd_list, 0, 1, vertex_buffers, offsets);
    }

    // Shader arguments
    {
        auto const& pso_node = _globals.pool_pipeline_states->get(draw.pipeline_state);
        pipeline_layout const& pipeline_layout = *pso_node.associated_pipeline_layout;

        for (uint8_t i = 0; i < draw.shader_arguments.size(); ++i)
        {
            auto const& arg = draw.shader_arguments[i];
            auto const& arg_vis = pipeline_layout.descriptor_set_visibilities[i];

            if (arg.constant_buffer != handle::null_resource)
            {
                // Unconditionally set the CBV

                _state_cache->touch_resource_in_shader(arg.constant_buffer, arg_vis);

                auto const cbv_desc_set = _globals.pool_resources->getRawCBVDescriptorSet(arg.constant_buffer);
                vkCmdBindDescriptorSets(_cmd_list, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout.raw_layout, i + limits::max_shader_arguments, 1,
                                        &cbv_desc_set, 1, &arg.constant_buffer_offset);
            }

            // Set the shader view if it has changed
            if (_bound.shader_views[i] != arg.shader_view)
            {
                _bound.shader_views[i] = arg.shader_view;
                if (arg.shader_view != handle::null_shader_view)
                {
                    // touch all contained resources in the state cache
                    for (auto const res : _globals.pool_shader_views->getResources(arg.shader_view))
                    {
                        // NOTE: this is pretty inefficient
                        _state_cache->touch_resource_in_shader(res, arg_vis);
                    }

                    auto const sv_desc_set = _globals.pool_shader_views->get(arg.shader_view);
                    vkCmdBindDescriptorSets(_cmd_list, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout.raw_layout, i, 1, &sv_desc_set, 0, nullptr);
                }
            }
        }
    }

    // Draw command
    if (draw.index_buffer.is_valid())
    {
        vkCmdDrawIndexed(_cmd_list, draw.num_indices, 1, 0, 0, 0);
    }
    else
    {
        vkCmdDraw(_cmd_list, draw.num_indices, 1, 0, 0);
    }
}

void pr::backend::vk::command_list_translator::execute(const pr::backend::cmd::end_render_pass&)
{
    // do nothing
}

void pr::backend::vk::command_list_translator::execute(const pr::backend::cmd::transition_resources& transition_res)
{
    barrier_bundle<limits::max_resource_transitions, limits::max_resource_transitions, limits::max_resource_transitions> barriers;

    for (auto const& transition : transition_res.transitions)
    {
        resource_state before;
        VkPipelineStageFlags before_dep;
        bool before_known = _state_cache->transition_resource(transition.resource, transition.target_state, before, before_dep);

        if (before_known && before != transition.target_state)
        {
            // The transition is neither the implicit initial one, nor redundant

            // TODO: if the target state requires a pipeline stage in order to resolve
            // this currently falls back to VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT
            // there are two ways to fix this:
            // defer all barriers requiring shader info until the draw call (where shader views are updated)
            // or require an explicit target shader domain in cmd::transition_resources

            state_change const change = state_change(before, transition.target_state, before_dep, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

            if (_globals.pool_resources->isImage(transition.resource))
            {
                auto const& img_info = _globals.pool_resources->getImageInfo(transition.resource);
                barriers.add_image_barrier(img_info.raw_image, change, util::to_native_image_aspect(img_info.pixel_format), img_info.num_mips,
                                           img_info.num_array_layers);
            }
            else
            {
                auto const& buf_info = _globals.pool_resources->getBufferInfo(transition.resource);
                barriers.add_buffer_barrier(buf_info.raw_buffer, change, buf_info.width);
            }
        }
    }

    barriers.record(_cmd_list);
}

void pr::backend::vk::command_list_translator::execute(const pr::backend::cmd::copy_buffer& copy_buf) {}

void pr::backend::vk::command_list_translator::execute(const pr::backend::cmd::copy_buffer_to_texture& copy_text) {}
