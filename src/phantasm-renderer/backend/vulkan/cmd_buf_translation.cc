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
#include "resources/transition_barrier.hh"

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
    if (_bound.update_pso(draw.pipeline_state))
    {
        // a new handle::pipeline_state invalidates (!= always changes)
        //      - The bound pipeline layout
        //      - The bound render pass
        //      - The bound pipeline

        auto const& pso_node = _globals.pool_pipeline_states->get(draw.pipeline_state);

        _bound.update_pipeline_layout(pso_node.associated_pipeline_layout->raw_layout);

        auto const render_pass = _globals.pool_pipeline_states->getOrCreateRenderPass(pso_node, _bound.current_render_pass);
        if (render_pass != _bound.raw_render_pass)
        {
            if (_bound.raw_render_pass != nullptr)
            {
                vkCmdEndRenderPass(_cmd_list);
            }

            // a render pass always changes
            //      - The framebuffer
            //      - The vkCmdBeginRenderPass/vkCmdEndRenderPass state
            _bound.raw_render_pass = render_pass;

            // create a new framebuffer
            {
                // the image views used in this framebuffer
                cc::capped_vector<VkImageView, limits::max_render_targets + 1> fb_image_views;
                // the image views used in this framebuffer, EXCLUDING possible backbuffer views
                // these are the ones which will get deleted alongside this framebuffer
                cc::capped_vector<VkImageView, limits::max_render_targets + 1> fb_image_views_to_clean_up;

                for (auto const& rt : _bound.current_render_pass.render_targets)
                {
                    if (_globals.pool_resources->isBackbuffer(rt.sve.resource))
                    {
                        fb_image_views.push_back(_globals.pool_resources->getBackbufferView());
                    }
                    else
                    {
                        fb_image_views.push_back(_globals.pool_shader_views->makeImageView(rt.sve));
                        fb_image_views_to_clean_up.push_back(fb_image_views.back());
                    }
                }

                if (_bound.current_render_pass.depth_target.sve.resource != handle::null_resource)
                {
                    fb_image_views.push_back(_globals.pool_shader_views->makeImageView(_bound.current_render_pass.depth_target.sve));
                    fb_image_views_to_clean_up.push_back(fb_image_views.back());
                }

                VkFramebufferCreateInfo fb_info = {};
                fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                fb_info.pNext = nullptr;
                fb_info.renderPass = render_pass;
                fb_info.attachmentCount = static_cast<unsigned>(fb_image_views.size());
                fb_info.pAttachments = fb_image_views.data();
                fb_info.width = static_cast<unsigned>(_bound.current_render_pass.viewport.width);
                fb_info.height = static_cast<unsigned>(_bound.current_render_pass.viewport.height);
                fb_info.layers = 1;

                // Create the framebuffer
                PR_VK_VERIFY_SUCCESS(vkCreateFramebuffer(_globals.device, &fb_info, nullptr, &_bound.raw_framebuffer));

                // Associate the framebuffer and all created image views with the current command list so they will get cleaned up
                _globals.pool_cmd_lists->addAssociatedFramebuffer(_cmd_list_handle, _bound.raw_framebuffer, fb_image_views_to_clean_up);
            }

            // begin a new render pass
            {
                VkRenderPassBeginInfo rp_begin_info = {};
                rp_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                rp_begin_info.renderPass = render_pass;
                rp_begin_info.framebuffer = _bound.raw_framebuffer;
                rp_begin_info.renderArea.offset = {0, 0};
                rp_begin_info.renderArea.extent.width = static_cast<uint32_t>(_bound.current_render_pass.viewport.width);
                rp_begin_info.renderArea.extent.height = static_cast<uint32_t>(_bound.current_render_pass.viewport.height);

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

                util::set_viewport(_cmd_list, _bound.current_render_pass.viewport);
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
    }

    // Vertex buffer
    if (draw.vertex_buffer != _bound.vertex_buffer)
    {
        _bound.vertex_buffer = draw.vertex_buffer;
        if (draw.vertex_buffer.is_valid())
        {
            VkBuffer vertex_buffers[] = {_globals.pool_resources->getRawBuffer(draw.vertex_buffer)};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(_cmd_list, 0, 1, vertex_buffers, offsets);
        }
    }

    // Shader arguments
    {
        auto const& pso_node = _globals.pool_pipeline_states->get(draw.pipeline_state);
        pipeline_layout const& pipeline_layout = *pso_node.associated_pipeline_layout;

        if (pipeline_layout.has_push_constants())
        {
            vkCmdPushConstants(_cmd_list, pipeline_layout.raw_layout, pipeline_layout.push_constant_stages, 0, sizeof(draw.root_constants), draw.root_constants);
        }

        for (uint8_t i = 0; i < draw.shader_arguments.size(); ++i)
        {
            auto& bound_arg = _bound.shader_args[i];
            auto const& arg = draw.shader_arguments[i];

            if (arg.constant_buffer != handle::null_resource)
            {
                if (bound_arg.update_cbv(arg.constant_buffer, arg.constant_buffer_offset))
                {
                    auto const cbv_desc_set = _globals.pool_resources->getRawCBVDescriptorSet(arg.constant_buffer);
                    vkCmdBindDescriptorSets(_cmd_list, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout.raw_layout, i + limits::max_shader_arguments,
                                            1, &cbv_desc_set, 1, &arg.constant_buffer_offset);
                }
            }

            // Set the shader view if it has changed
            if (bound_arg.update_shader_view(arg.shader_view))
            {
                if (arg.shader_view != handle::null_shader_view)
                {
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

void pr::backend::vk::command_list_translator::execute(const pr::backend::cmd::dispatch& dispatch)
{
    if (_bound.update_pso(dispatch.pipeline_state))
    {
        // a new handle::pipeline_state invalidates (!= always changes)
        //      - The bound pipeline layout
        //      - The bound pipeline
        auto const& pso_node = _globals.pool_pipeline_states->get(dispatch.pipeline_state);

        _bound.update_pipeline_layout(pso_node.associated_pipeline_layout->raw_layout);

        vkCmdBindPipeline(_cmd_list, VK_PIPELINE_BIND_POINT_COMPUTE, pso_node.raw_pipeline);
    }

    // Shader arguments
    {
        auto const& pso_node = _globals.pool_pipeline_states->get(dispatch.pipeline_state);
        pipeline_layout const& pipeline_layout = *pso_node.associated_pipeline_layout;

        if (pipeline_layout.has_push_constants())
        {
            vkCmdPushConstants(_cmd_list, pipeline_layout.raw_layout, pipeline_layout.push_constant_stages, 0, sizeof(dispatch.root_constants),
                               dispatch.root_constants);
        }

        for (uint8_t i = 0; i < dispatch.shader_arguments.size(); ++i)
        {
            auto& bound_arg = _bound.shader_args[i];
            auto const& arg = dispatch.shader_arguments[i];

            if (arg.constant_buffer != handle::null_resource)
            {
                // Set the CBV / offset if it has changed
                if (bound_arg.update_cbv(arg.constant_buffer, arg.constant_buffer_offset))
                {
                    auto const cbv_desc_set = _globals.pool_resources->getRawCBVDescriptorSetCompute(arg.constant_buffer);
                    CC_ASSERT(cbv_desc_set != nullptr);
                    vkCmdBindDescriptorSets(_cmd_list, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout.raw_layout, i + limits::max_shader_arguments,
                                            1, &cbv_desc_set, 1, &arg.constant_buffer_offset);
                }
            }

            if (bound_arg.update_shader_view(arg.shader_view))
            {
                // Set the shader view if it has changed
                if (arg.shader_view != handle::null_shader_view)
                {
                    VkDescriptorSet sv_desc_set = _globals.pool_shader_views->get(arg.shader_view);
                    vkCmdBindDescriptorSets(_cmd_list, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout.raw_layout, i, 1, &sv_desc_set, 0, nullptr);
                }
            }
        }
    }


    // Dispatch command
    vkCmdDispatch(_cmd_list, dispatch.dispatch_x, dispatch.dispatch_y, dispatch.dispatch_z);
}

void pr::backend::vk::command_list_translator::execute(const pr::backend::cmd::end_render_pass&)
{
    if (_bound.raw_render_pass != nullptr)
    {
        vkCmdEndRenderPass(_cmd_list);
        _bound.raw_render_pass = nullptr;
    }
}

void pr::backend::vk::command_list_translator::execute(const pr::backend::cmd::transition_resources& transition_res)
{
    // NOTE: Barriers adhere to some special rules in the vulkan backend:
    // 1. They must not occur within an active render pass
    // 2. Render passes always expect all render targets to be transitioned to resource_state::render_target
    //    and depth targets to be transitioned to resource_state::depth_write
    CC_ASSERT(_bound.raw_render_pass == nullptr && "Vulkan resource transitions must not occur during render passes");

    barrier_bundle<limits::max_resource_transitions, limits::max_resource_transitions, limits::max_resource_transitions> barriers;

    for (auto const& transition : transition_res.transitions)
    {
        auto const after_dep = util::to_pipeline_stage_dependency(transition.target_state, util::to_pipeline_stage_flags_bitwise(transition.dependant_shaders));
        CC_ASSERT(after_dep != 0 && "Transition shader dependencies must be specified if transitioning to a CBV/SRV/UAV");

        resource_state before;
        VkPipelineStageFlags before_dep;
        bool before_known = _state_cache->transition_resource(transition.resource, transition.target_state, after_dep, before, before_dep);

        if (before_known && before != transition.target_state)
        {
            // The transition is neither the implicit initial one, nor redundant
            state_change const change = state_change(before, transition.target_state, before_dep, after_dep);

            // NOTE: in both cases we transition the entire resource (all subresources in D3D12 terms),
            // using stored information from the resource pool (img_info / buf_info respectively)
            if (_globals.pool_resources->isImage(transition.resource))
            {
                auto const& img_info = _globals.pool_resources->getImageInfo(transition.resource);
                barriers.add_image_barrier(img_info.raw_image, change, util::to_native_image_aspect(img_info.pixel_format));
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

void pr::backend::vk::command_list_translator::execute(const pr::backend::cmd::transition_image_slices& transition_images)
{
    // Image slice transitions are entirely explicit, and require the user to synchronize before/after resource states
    // NOTE: we do not update the master state as it does not encompass subresource states

    barrier_bundle<limits::max_resource_transitions> barriers;

    for (auto const& transition : transition_images.transitions)
    {
        auto const before_dep
            = util::to_pipeline_stage_dependency(transition.source_state, util::to_pipeline_stage_flags_bitwise(transition.source_dependencies));
        auto const after_dep
            = util::to_pipeline_stage_dependency(transition.target_state, util::to_pipeline_stage_flags_bitwise(transition.target_dependencies));

        state_change const change = state_change(transition.source_state, transition.target_state, before_dep, after_dep);

        CC_ASSERT(_globals.pool_resources->isImage(transition.resource));
        auto const& img_info = _globals.pool_resources->getImageInfo(transition.resource);
        barriers.add_image_barrier(img_info.raw_image, change, util::to_native_image_aspect(img_info.pixel_format),
                                   static_cast<unsigned>(transition.mip_level), static_cast<unsigned>(transition.array_slice));
    }

    barriers.record(_cmd_list);
}

void pr::backend::vk::command_list_translator::execute(const pr::backend::cmd::copy_buffer& copy_buf)
{
    auto const src_buffer = _globals.pool_resources->getRawBuffer(copy_buf.source);
    auto const dest_buffer = _globals.pool_resources->getRawBuffer(copy_buf.destination);

    VkBufferCopy region = {};
    region.size = copy_buf.size;
    region.srcOffset = copy_buf.source_offset;
    region.dstOffset = copy_buf.dest_offset;
    vkCmdCopyBuffer(_cmd_list, src_buffer, dest_buffer, 1, &region);
}

void pr::backend::vk::command_list_translator::execute(const pr::backend::cmd::copy_texture& copy_text)
{
    auto const& src_image_info = _globals.pool_resources->getImageInfo(copy_text.source);
    auto const& dest_image_info = _globals.pool_resources->getImageInfo(copy_text.destination);

    VkImageCopy copy = {};
    copy.srcSubresource.aspectMask = util::to_native_image_aspect(src_image_info.pixel_format);
    copy.srcSubresource.baseArrayLayer = copy_text.src_array_index;
    copy.srcSubresource.layerCount = copy_text.num_array_slices;
    copy.srcSubresource.mipLevel = copy_text.src_mip_index;
    copy.dstSubresource.aspectMask = util::to_native_image_aspect(dest_image_info.pixel_format);
    copy.dstSubresource.baseArrayLayer = copy_text.dest_array_index;
    copy.dstSubresource.layerCount = copy_text.num_array_slices;
    copy.dstSubresource.mipLevel = copy_text.dest_mip_index;
    copy.extent.width = copy_text.width;
    copy.extent.height = copy_text.height;
    copy.extent.depth = copy_text.num_array_slices;

    vkCmdCopyImage(_cmd_list, src_image_info.raw_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dest_image_info.raw_image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
}

void pr::backend::vk::command_list_translator::execute(const pr::backend::cmd::copy_buffer_to_texture& copy_text)
{
    auto const src_buffer = _globals.pool_resources->getRawBuffer(copy_text.source);
    auto const& dest_image_info = _globals.pool_resources->getImageInfo(copy_text.destination);

    VkBufferImageCopy region = {};
    region.bufferOffset = uint32_t(copy_text.source_offset);
    region.imageSubresource.aspectMask = util::to_native_image_aspect(dest_image_info.pixel_format);
    region.imageSubresource.baseArrayLayer = copy_text.dest_array_index;
    region.imageSubresource.layerCount = 1;
    region.imageSubresource.mipLevel = copy_text.dest_mip_index;
    region.imageExtent.width = copy_text.dest_width;
    region.imageExtent.height = copy_text.dest_height;
    region.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(_cmd_list, src_buffer, dest_image_info.raw_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void pr::backend::vk::command_list_translator::execute(const pr::backend::cmd::debug_marker& marker)
{
    VkDebugUtilsLabelEXT label = {};
    label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    label.pLabelName = marker.string_literal;

    // this function pointer is not available in all configurations
    if (vkCmdInsertDebugUtilsLabelEXT)
        vkCmdInsertDebugUtilsLabelEXT(_cmd_list, &label);
}
