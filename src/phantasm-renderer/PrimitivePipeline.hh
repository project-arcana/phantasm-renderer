#pragma once

#include <phantasm-hardware-interface/arguments.hh>
#include <phantasm-hardware-interface/commands.hh>

#include <phantasm-renderer/argument.hh>
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
    PrimitivePipeline(Frame* parent, phi::handle::pipeline_state pso);

    void draw(unsigned num_vertices);
    void draw(buffer const& vertex_buffer);
    void draw_indexed(buffer const& vertex_buffer, buffer const& index_buffer);

    // shitty WIP API
    void add_argument(baked_shader_view const& sv);
    void add_argument(baked_shader_view const& sv, buffer const& constant_buffer, uint32_t constant_buffer_offset = 0);
    void add_argument(buffer const& constant_buffer, uint32_t constant_buffer_offset = 0);

    void pop_argument();
    void clear_arguments();

    template <class T>
    void write_root_constants(T const& data)
    {
        mDrawCmd.write_root_constants<T>(data);
    }

    PrimitivePipeline(PrimitivePipeline&&) noexcept = default;
    PrimitivePipeline(PrimitivePipeline const&) = delete;

private:
    Frame* mParent;
    phi::handle::pipeline_state mPSO;
    phi::cmd::draw mDrawCmd;
};
}
