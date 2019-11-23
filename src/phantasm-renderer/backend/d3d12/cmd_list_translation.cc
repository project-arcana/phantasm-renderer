#include "cmd_list_translation.hh"

#include <phantasm-renderer/backend/detail/byte_util.hh>

#include "common/dxgi_format.hh"
#include "common/native_enum.hh"

void pr::backend::d3d12::command_list_translator::initialize(ID3D12Device* device,
                                                             pr::backend::d3d12::ShaderViewPool* sv_pool,
                                                             pr::backend::d3d12::ResourcePool* resource_pool,
                                                             pr::backend::d3d12::PipelineStateObjectPool* pso_pool)
{
    _globals.initialize(device, sv_pool, resource_pool, pso_pool);
    _thread_local.initialize(*_globals.device);
}

void pr::backend::d3d12::command_list_translator::translateCommandList(ID3D12GraphicsCommandList* list,
                                                                       ::pr::backend::detail::incomplete_state_cache* state_cache,
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

        auto* const resource = _globals.pool_resources->getRawResource(rt.resource);
        auto const rtv = dynamic_rtvs.get_index(i);

        // create the default RTV on the fly
        // TODO not a default RTV
        _globals.device->CreateRenderTargetView(resource, nullptr, rtv);

        if (rt.clear_type == cmd::begin_render_pass::rt_clear_type::clear)
        {
            _cmd_list->ClearRenderTargetView(rtv, rt.clear_value, 0, nullptr);
        }
    }

    resource_view_cpu_only dynamic_dsv;
    if (begin_rp.depth_target.resource != handle::null_resource)
    {
        dynamic_dsv = _thread_local.lin_alloc_dsvs.allocate(1u);
        auto* const resource = _globals.pool_resources->getRawResource(begin_rp.depth_target.resource);

        // create the default DSV on the fly
        // TODO not a default DSV
        _globals.device->CreateDepthStencilView(resource, nullptr, dynamic_dsv.get_start());

        if (begin_rp.depth_target.clear_type == cmd::begin_render_pass::rt_clear_type::clear)
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
            _cmd_list->IASetIndexBuffer(nullptr);
        }
    }

    // Vertex buffer
    if (draw.vertex_buffer != _bound.vertex_buffer)
    {
        _bound.vertex_buffer = draw.vertex_buffer;
        auto const vbv = _globals.pool_resources->getVertexBufferView(draw.vertex_buffer);
        _cmd_list->IASetVertexBuffers(0, 1, &vbv);
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

            if (map.descriptor_table_param != uint32_t(-1))
            {
                // Set the shader view if it has changed
                if (_bound.shader_views[i] != arg.shader_view)
                {
                    _bound.shader_views[i] = arg.shader_view;
                    auto const sv_desc_table = _globals.pool_shader_views->getGPUStart(arg.shader_view);
                    _cmd_list->SetGraphicsRootDescriptorTable(map.descriptor_table_param, sv_desc_table);
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
            auto& barrier = barriers.emplace_back();
            util::populate_barrier_desc(barrier, _globals.pool_resources->getRawResource(transition.resource), util::to_native(before),
                                        util::to_native(transition.target_state));
        }
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

void pr::backend::d3d12::command_list_translator::execute(const pr::backend::cmd::copy_buffer_to_texture& copy_text)
{
    auto const format_dxgi = util::to_dxgi_format(copy_text.texture_format);

    D3D12_SUBRESOURCE_FOOTPRINT footprint;
    footprint.Format = format_dxgi;
    footprint.Width = copy_text.mip_width;
    footprint.Height = copy_text.mip_height;
    footprint.Depth = 1;
    footprint.RowPitch = copy_text.row_pitch;
    //    footprint.RowPitch = mem::align_up(util::get_dxgi_bytes_per_pixel(format_dxgi) * copy_text.mip_width, 256);

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT placed_footprint;
    placed_footprint.Offset = copy_text.source_offset;
    placed_footprint.Footprint = footprint;

    CD3DX12_TEXTURE_COPY_LOCATION const source(_globals.pool_resources->getRawResource(copy_text.source), placed_footprint);
    CD3DX12_TEXTURE_COPY_LOCATION const dest(_globals.pool_resources->getRawResource(copy_text.destination), copy_text.subresource_index);
    _cmd_list->CopyTextureRegion(&dest, 0, 0, 0, &source, nullptr);
}
