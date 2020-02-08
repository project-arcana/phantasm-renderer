#pragma once

#include <phantasm-hardware-interface/arguments.hh>

#include <phantasm-renderer/format.hh>
#include <phantasm-renderer/fwd.hh>
#include <phantasm-renderer/reflection/resources.hh>
#include <phantasm-renderer/resource_types.hh>

namespace pr
{
class Frame;

class PrimitivePipeline
{
public:

    void draw(buffer const& vertex_buffer);

    void draw_indexed(buffer const& vertex_buffer, buffer const& index_buffer);

    // TODO: multiple buffers
    //    template <class BufferT>
    //    void draw(UnboundResources const&... resources, Buffer<BufferT> const& vertexBuffer)
    //    {
    //        //        static_assert(sizeof...(UnboundResources) == 0, "trying to draw with unbound resources"); // isn't this right?
    //        static_assert(std::is_same_v<BufferT, VertexT[]>, "incompatible vertex types");

    //        phi::cmd::draw dcmd;
    //        dcmd.init(mHandlePSO, vertexBuffer.getNumElements(), vertexBuffer.getHandle());
    //        mParentWriter->add_command(dcmd);
    //    }

    //    // TODO: more index types
    //    template <class BufferT>
    //    void drawIndexed(UnboundResources const&... resources, Buffer<BufferT> const& vertexBuffer, Buffer<uint32_t[]> const& indexBuffer)
    //    {
    //        static_assert(sizeof...(UnboundResources) == 0, "trying to draw with unbound resources");
    //        static_assert(std::is_same_v<BufferT, VertexT[]>, "incompatible vertex types");

    //        phi::cmd::draw dcmd;
    //        dcmd.init(mHandlePSO, indexBuffer.getNumElements(), vertexBuffer.getHandle(), indexBuffer.getHandle());
    //        mParentWriter->add_command(dcmd);
    //    }

    //    template <class... NewResources>
    //    bound_pipeline_type<VertexT, FragmentF, append_resource_list<BoundResourceList, NewResources...>,
    //    remove_resource_prefix<resource_list<UnboundResources...>, NewResources...>> bind(NewResources const&... resources)
    //    {
    //        static_assert(sizeof...(NewResources) <= sizeof...(UnboundResources), "trying to bind too many resources");
    //        return {mCtx, mParentWriter, mShaderVertex, mShaderPixel, mConfig}; // TODO
    //    }

    // public:
    //    PrimitivePipeline(Context* ctx, growing_writer* writer, VertexShader<VertexT> vs, FragmentShader<FragmentF> ps, phi::pipeline_config const& config)
    //      : mCtx(ctx), mParentWriter(writer), mShaderVertex(vs), mShaderPixel(ps), mConfig(config)
    //    {
    //        if constexpr (sizeof...(UnboundResources) == 0) // TODO: this doesn't work
    //        {
    //            // fully bound pipeline, lookup PSO
    //            cc::capped_vector<phi::arg::shader_arg_shape, phi::limits::max_shader_arguments> shader_arg_shapes;

    //            // TODO:
    //            //(shader_arg_shapes.push_back(reflect_arg_shape<BoundResourceList>()), ...); // semantically this

    //            phi::arg::framebuffer_config fbconf;
    //            fbconf.add_render_target(FragmentF);

    //            cc::array const shaders = {phi::arg::graphics_shader{mShaderVertex.binary, phi::shader_stage::vertex},
    //                                       phi::arg::graphics_shader{mShaderPixel.binary, phi::shader_stage::pixel}};

    //            phi::arg::vertex_format const vert_fmt = {VertexShader<VertexT>::sAttributes, sizeof(VertexT)};

    //            // TODO: existence of root constants
    //            mHandlePSO = mCtx->acquirePSO(vert_fmt, fbconf, shader_arg_shapes, false, shaders, mConfig);
    //        }
    //    }

private:
    Frame* mParent;
    phi::handle::pipeline_state mPSO;
};
}
