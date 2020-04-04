#include "pipeline_builder.hh"

#include <clean-core/assert.hh>

#include <phantasm-renderer/Context.hh>

pr::compute_pipeline_state pr::pipeline_builder::make_compute()
{
    CC_ASSERT(_shaders.size() == 1 && _shaders[0].stage == phi::shader_stage::compute && "Attempted to create compute pipeline without compute shader");
    return compute_pipeline_state{{{_parent->get_backend()->createComputePipelineState(_arg_shapes, _shaders[0].binary, _has_root_consts)}}, _parent};
}

pr::graphics_pipeline_state pr::pipeline_builder::make_graphics()
{
    return graphics_pipeline_state{{{_parent->get_backend()->createPipelineState({_vertex_attributes, _vertex_size_bytes}, _framebuffer_config,
                                                                                 _arg_shapes, _has_root_consts, _shaders, _graphics_config)}},
                                   _parent};
}
