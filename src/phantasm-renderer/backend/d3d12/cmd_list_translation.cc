#include "cmd_list_translation.hh"

#include <phantasm-renderer/backend/detail/byte_util.hh>
#include <phantasm-renderer/backend/detail/format_size.hh>
#include <phantasm-renderer/backend/detail/incomplete_state_cache.hh>

#include "Swapchain.hh"
#include "common/dxgi_format.hh"
#include "common/native_enum.hh"
#include "common/util.hh"
#include "pools/pso_pool.hh"
#include "pools/resource_pool.hh"
#include "pools/root_sig_cache.hh"
#include "pools/shader_view_pool.hh"

void pr::backend::d3d12::command_list_translator::initialize(ID3D12Device* device,
                                                             pr::backend::d3d12::ShaderViewPool* sv_pool,
                                                             pr::backend::d3d12::ResourcePool* resource_pool,
                                                             pr::backend::d3d12::PipelineStateObjectPool* pso_pool)
{
    _globals.initialize(device, sv_pool, resource_pool, pso_pool);
    _thread_local.initialize(*_globals.device);
}

void pr::backend::d3d12::command_list_translator::translateCommandList(ID3D12GraphicsCommandList* list,
                                                                       backend::detail::incomplete_state_cache* state_cache,
                                                                       std::byte* buffer,
                                                                       size_t buffer_size)
{
    _cmd_list = list;
    _state_cache = state_cache;

    _bound.reset();
    _state_cache->reset();

    auto const gpu_heaps = _globals.pool_shader_views->getGPURelevantHeaps();
    _cmd_list->SetDescriptorHeaps(UINT(gpu_heaps.size()), gpu_heaps.data());

    // translate all contained commands
    command_stream_parser parser(buffer, buffer_size);
    for (auto const& cmd : parser)
        cmd::detail::dynamic_dispatch(cmd, *this);

    // close the list
    _cmd_list->Close();

    // done
}

void pr::backend::d3d12::command_list_translator::execute(const pr::backend::cmd::begin_render_pass& begin_rp)
{
    util::set_viewport(_cmd_list, begin_rp.viewport);

    resource_view_cpu_only const dynamic_rtvs = _thread_local.lin_alloc_rtvs.allocate(begin_rp.render_targets.size());

    for (uint8_t i = 0; i < begin_rp.render_targets.size(); ++i)
    {
        auto const& rt = begin_rp.render_targets[i];
        auto const& sve = rt.sve;

        auto* const resource = _globals.pool_resources->getRawResource(sve.resource);
        auto const rtv = dynamic_rtvs.get_index(i);

        // create the default RTV on the fly
        if (_globals.pool_resources->isBackbuffer(sve.resource))
        {
            // Create a default RTV for the backbuffer
            _globals.device->CreateRenderTargetView(resource, nullptr, rtv);
        }
        else
        {
            // Create an RTV based on the supplied info
            auto const rtv_desc = util::create_rtv_desc(sve);
            _globals.device->CreateRenderTargetView(resource, &rtv_desc, rtv);
        }

        if (rt.clear_type == rt_clear_type::clear)
        {
            _cmd_list->ClearRenderTargetView(rtv, rt.clear_value, 0, nullptr);
        }
    }

    resource_view_cpu_only dynamic_dsv;
    if (begin_rp.depth_target.sve.resource != handle::null_resource)
    {
        dynamic_dsv = _thread_local.lin_alloc_dsvs.allocate(1u);
        auto* const resource = _globals.pool_resources->getRawResource(begin_rp.depth_target.sve.resource);

        // Create an DSV based on the supplied info
        auto const dsv_desc = util::create_dsv_desc(begin_rp.depth_target.sve);
        _globals.device->CreateDepthStencilView(resource, &dsv_desc, dynamic_dsv.get_start());

        if (begin_rp.depth_target.clear_type == rt_clear_type::clear)
        {
            _cmd_list->ClearDepthStencilView(dynamic_dsv.get_start(), D3D12_CLEAR_FLAG_DEPTH, begin_rp.depth_target.clear_value_depth,
                                             begin_rp.depth_target.clear_value_stencil, 0, nullptr);
        }
    }

    // set the render targets
    _cmd_list->OMSetRenderTargets(begin_rp.render_targets.size(), begin_rp.render_targets.size() > 0 ? &dynamic_rtvs.get_start() : nullptr, true,
                                  dynamic_dsv.is_valid() ? &dynamic_dsv.get_start() : nullptr);

    // reset the linear allocators
    _thread_local.lin_alloc_rtvs.reset();
    _thread_local.lin_alloc_dsvs.reset();
}

void pr::backend::d3d12::command_list_translator::execute(const pr::backend::cmd::draw& draw)
{
    auto const& pso_node = _globals.pool_pipeline_states->get(draw.pipeline_state);

    // PSO
    if (draw.pipeline_state != _bound.pipeline_state)
    {
        _bound.pipeline_state = draw.pipeline_state;
        _cmd_list->SetPipelineState(pso_node.raw_pso);
    }

    // Root signature
    if (pso_node.associated_root_sig->raw_root_sig != _bound.raw_root_sig)
    {
        // A new root signature invalidates bound shader arguments
        _bound.set_root_sig(pso_node.associated_root_sig->raw_root_sig);
        _cmd_list->SetGraphicsRootSignature(_bound.raw_root_sig);
        _cmd_list->IASetPrimitiveTopology(pso_node.primitive_topology);
    }

    // Index buffer (optional)
    if (draw.index_buffer != _bound.index_buffer)
    {
        _bound.index_buffer = draw.index_buffer;
        if (draw.index_buffer.is_valid())
        {
            auto const ibv = _globals.pool_resources->getIndexBufferView(draw.index_buffer);
            _cmd_list->IASetIndexBuffer(&ibv);
        }
        else
        {
            // TODO: does this even make sense?
            //  _cmd_list->IASetIndexBuffer(nullptr);
        }
    }

    // Vertex buffer
    if (draw.vertex_buffer != _bound.vertex_buffer)
    {
        _bound.vertex_buffer = draw.vertex_buffer;
        if (draw.vertex_buffer.is_valid())
        {
            auto const vbv = _globals.pool_resources->getVertexBufferView(draw.vertex_buffer);
            _cmd_list->IASetVertexBuffers(0, 1, &vbv);
        }
    }

    // Shader arguments
    {
        auto const& root_sig = *pso_node.associated_root_sig;

        for (uint8_t i = 0; i < draw.shader_arguments.size(); ++i)
        {
            auto const& arg = draw.shader_arguments[i];
            auto const& map = root_sig.argument_maps[i];

            if (map.cbv_param != uint32_t(-1))
            {
                // Unconditionally set the CBV
                auto const cbv = _globals.pool_resources->getConstantBufferView(arg.constant_buffer);
                _cmd_list->SetGraphicsRootConstantBufferView(map.cbv_param, cbv.BufferLocation + arg.constant_buffer_offset);
            }

            // Set the shader view if it has changed
            if (_bound.shader_views[i] != arg.shader_view)
                _bound.shader_views[i] = arg.shader_view;
            {
                if (map.srv_uav_table_param != uint32_t(-1))
                {
                    auto const sv_desc_table = _globals.pool_shader_views->getSRVUAVGPUHandle(arg.shader_view);
                    _cmd_list->SetGraphicsRootDescriptorTable(map.srv_uav_table_param, sv_desc_table);
                }

                if (map.sampler_table_param != uint32_t(-1))
                {
                    auto const sampler_desc_table = _globals.pool_shader_views->getSamplerGPUHandle(arg.shader_view);
                    _cmd_list->SetGraphicsRootDescriptorTable(map.sampler_table_param, sampler_desc_table);
                }
            }
        }
    }

    // Draw command
    if (draw.index_buffer.is_valid())
    {
        _cmd_list->DrawIndexedInstanced(draw.num_indices, 1, 0, 0, 0);
    }
    else
    {
        _cmd_list->DrawInstanced(draw.num_indices, 1, 0, 0);
    }
}

void pr::backend::d3d12::command_list_translator::execute(const pr::backend::cmd::dispatch& dispatch)
{
    auto const& pso_node = _globals.pool_pipeline_states->get(dispatch.pipeline_state);

    // PSO
    if (dispatch.pipeline_state != _bound.pipeline_state)
    {
        _bound.pipeline_state = dispatch.pipeline_state;
        _cmd_list->SetPipelineState(pso_node.raw_pso);
    }

    // Root signature
    if (pso_node.associated_root_sig->raw_root_sig != _bound.raw_root_sig)
    {
        // A new root signature invalidates bound shader arguments
        _bound.set_root_sig(pso_node.associated_root_sig->raw_root_sig);
        _cmd_list->SetComputeRootSignature(_bound.raw_root_sig);
    }

    // Shader arguments
    {
        auto const& root_sig = *pso_node.associated_root_sig;

        // root constants
        auto const root_constant_param = root_sig.argument_maps[0].root_const_param;
        if (root_constant_param != unsigned(-1))
        {
            static_assert(sizeof(dispatch.root_constants) % sizeof(DWORD32) == 0, "root constant size not divisible by dword32 size");
            _cmd_list->SetComputeRoot32BitConstants(root_constant_param, sizeof(dispatch.root_constants) / sizeof(DWORD32), dispatch.root_constants, 0);
        }

        // regular shader arguments
        for (uint8_t i = 0; i < dispatch.shader_arguments.size(); ++i)
        {
            auto const& arg = dispatch.shader_arguments[i];
            auto const& map = root_sig.argument_maps[i];

            if (map.cbv_param != uint32_t(-1))
            {
                // Unconditionally set the CBV
                auto const cbv = _globals.pool_resources->getConstantBufferView(arg.constant_buffer);
                _cmd_list->SetComputeRootConstantBufferView(map.cbv_param, cbv.BufferLocation + arg.constant_buffer_offset);
            }

            // Set the shader view if it has changed
            if (_bound.shader_views[i] != arg.shader_view)
                _bound.shader_views[i] = arg.shader_view;
            {
                if (map.srv_uav_table_param != uint32_t(-1))
                {
                    auto const sv_desc_table = _globals.pool_shader_views->getSRVUAVGPUHandle(arg.shader_view);
                    _cmd_list->SetComputeRootDescriptorTable(map.srv_uav_table_param, sv_desc_table);
                }

                if (map.sampler_table_param != uint32_t(-1))
                {
                    auto const sampler_desc_table = _globals.pool_shader_views->getSamplerGPUHandle(arg.shader_view);
                    _cmd_list->SetComputeRootDescriptorTable(map.sampler_table_param, sampler_desc_table);
                }
            }
        }
    }

    // Dispatch command
    _cmd_list->Dispatch(dispatch.dispatch_x, dispatch.dispatch_y, dispatch.dispatch_z);
}

void pr::backend::d3d12::command_list_translator::execute(const pr::backend::cmd::end_render_pass&)
{
    // do nothing
}

void pr::backend::d3d12::command_list_translator::execute(const pr::backend::cmd::transition_resources& transition_res)
{
    cc::capped_vector<D3D12_RESOURCE_BARRIER, limits::max_resource_transitions> barriers;

    for (auto const& transition : transition_res.transitions)
    {
        resource_state before;
        bool before_known = _state_cache->transition_resource(transition.resource, transition.target_state, before);

        if (before_known && before != transition.target_state)
        {
            // The transition is neither the implicit initial one, nor redundant
            barriers.push_back(util::get_barrier_desc(_globals.pool_resources->getRawResource(transition.resource), before, transition.target_state));
        }
    }

    if (!barriers.empty())
    {
        _cmd_list->ResourceBarrier(UINT(barriers.size()), barriers.data());
    }
}

void pr::backend::d3d12::command_list_translator::execute(const pr::backend::cmd::transition_image_slices& transition_images)
{
    // Image slice transitions are entirely explicit, and require the user to synchronize before/after resource states
    // NOTE: we do not update the master state as it does not encompass subresource states

    cc::capped_vector<D3D12_RESOURCE_BARRIER, limits::max_resource_transitions> barriers;
    for (auto const& transition : transition_images.transitions)
    {
        CC_ASSERT(_globals.pool_resources->isImage(transition.resource));
        auto const& img_info = _globals.pool_resources->getImageInfo(transition.resource);
        barriers.push_back(util::get_barrier_desc(_globals.pool_resources->getRawResource(transition.resource), transition.source_state,
                                                  transition.target_state, transition.mip_level, transition.array_slice, img_info.num_mips));
    }

    if (!barriers.empty())
    {
        _cmd_list->ResourceBarrier(UINT(barriers.size()), barriers.data());
    }
}

void pr::backend::d3d12::command_list_translator::execute(const pr::backend::cmd::copy_buffer& copy_buf)
{
    _cmd_list->CopyBufferRegion(_globals.pool_resources->getRawResource(copy_buf.destination), copy_buf.dest_offset,
                                _globals.pool_resources->getRawResource(copy_buf.source), copy_buf.source_offset, copy_buf.size);
}

void pr::backend::d3d12::command_list_translator::execute(const pr::backend::cmd::copy_texture& copy_text)
{
    auto const& src_info = _globals.pool_resources->getImageInfo(copy_text.source);
    auto const& dest_info = _globals.pool_resources->getImageInfo(copy_text.destination);

    for (auto array_offset = 0u; array_offset < copy_text.num_array_slices; ++array_offset)
    {
        auto const src_subres_index = copy_text.src_mip_index + (copy_text.src_array_index + array_offset) * src_info.num_mips;
        auto const dest_subres_index = copy_text.dest_mip_index + (copy_text.dest_array_index + array_offset) * dest_info.num_mips;

        CD3DX12_TEXTURE_COPY_LOCATION const source(_globals.pool_resources->getRawResource(copy_text.source), src_subres_index);
        CD3DX12_TEXTURE_COPY_LOCATION const dest(_globals.pool_resources->getRawResource(copy_text.destination), dest_subres_index);
        _cmd_list->CopyTextureRegion(&dest, 0, 0, 0, &source, nullptr);
    }
}

void pr::backend::d3d12::command_list_translator::execute(const pr::backend::cmd::copy_buffer_to_texture& copy_text)
{
    auto const& dest_info = _globals.pool_resources->getImageInfo(copy_text.destination);
    auto const pixel_bytes = backend::detail::pr_format_size_bytes(dest_info.pixel_format);
    auto const format_dxgi = util::to_dxgi_format(dest_info.pixel_format);

    D3D12_SUBRESOURCE_FOOTPRINT footprint;
    footprint.Format = format_dxgi;
    footprint.Width = copy_text.dest_width;
    footprint.Height = copy_text.dest_height;
    footprint.Depth = 1;
    footprint.RowPitch = mem::align_up(pixel_bytes * copy_text.dest_width, 256);

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT placed_footprint;
    placed_footprint.Offset = copy_text.source_offset;
    placed_footprint.Footprint = footprint;

    auto const subres_index = copy_text.dest_mip_index + copy_text.dest_array_index * dest_info.num_mips;

    CD3DX12_TEXTURE_COPY_LOCATION const source(_globals.pool_resources->getRawResource(copy_text.source), placed_footprint);
    CD3DX12_TEXTURE_COPY_LOCATION const dest(_globals.pool_resources->getRawResource(copy_text.destination), subres_index);
    _cmd_list->CopyTextureRegion(&dest, 0, 0, 0, &source, nullptr);
}

void pr::backend::d3d12::translator_thread_local_memory::initialize(ID3D12Device& device)
{
    lin_alloc_rtvs.initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 8);
    lin_alloc_dsvs.initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);
}
