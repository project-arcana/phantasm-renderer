#pragma once

#include <cstring>

#include <clean-core/capped_vector.hh>
#include <clean-core/utility.hh>

#include <typed-geometry/types/size.hh>

#include "detail/trivial_capped_vector.hh"
#include "limits.hh"
#include "types.hh"

namespace pr::backend
{
namespace cmd
{
namespace detail
{
#define PR_CMD_TYPE_VALUES        \
    PR_X(draw)                    \
    PR_X(dispatch)                \
    PR_X(transition_resources)    \
    PR_X(transition_image_slices) \
    PR_X(copy_buffer)             \
    PR_X(copy_buffer_to_texture)  \
    PR_X(begin_render_pass)       \
    PR_X(end_render_pass)

enum class cmd_type : uint8_t
{
#define PR_X(_val_) _val_,
    PR_CMD_TYPE_VALUES
#undef PR_X
};

enum class cmd_queue_type : uint8_t
{
    none = 0x00,
    copy = 0x01,
    compute = 0x02,
    graphics = 0x04,
    all = copy | compute | graphics
};

struct cmd_base
{
    cmd_type type;
    cmd_base(cmd_type t) : type(t) {}
};

template <cmd_type TYPE, cmd_queue_type QUEUE = cmd_queue_type::graphics>
struct typed_cmd : cmd_base
{
    static constexpr cmd_queue_type s_queue_type = QUEUE;
    typed_cmd() : cmd_base(TYPE) {}
};

}

template <class T, uint8_t N>
using cmd_vector = backend::detail::trivial_capped_vector<T, N>;

#define PR_DEFINE_CMD(_type_) struct _type_ final : detail::typed_cmd<detail::cmd_type::_type_>

PR_DEFINE_CMD(begin_render_pass)
{
    struct render_target_info
    {
        shader_view_element sve;
        float clear_value[4];
        rt_clear_type clear_type;
    };

    struct depth_stencil_info
    {
        shader_view_element sve;
        float clear_value_depth;
        uint8_t clear_value_stencil;
        rt_clear_type clear_type;
    };

    cmd_vector<render_target_info, limits::max_render_targets> render_targets;
    depth_stencil_info depth_target;
    tg::isize2 viewport;

public:
    // convenience

    void add_backbuffer_rt(handle::resource res)
    {
        render_targets.push_back(render_target_info{{}, {0.f, 0.f, 0.f, 1.f}, rt_clear_type::clear});
        render_targets.back().sve.init_as_backbuffer(res);
    }

    void add_2d_rt(handle::resource res, format pixel_format, rt_clear_type clear_op = rt_clear_type::clear, bool multisampled = false)
    {
        render_targets.push_back(render_target_info{{}, {0.f, 0.f, 0.f, 1.f}, clear_op});
        render_targets.back().sve.init_as_tex2d(res, pixel_format, multisampled);
    }

    void set_2d_depth_stencil(handle::resource res, format pixel_format, rt_clear_type clear_op = rt_clear_type::clear, bool multisampled = false)
    {
        depth_target = depth_stencil_info{{}, 1.f, 0, clear_op};
        depth_target.sve.init_as_tex2d(res, pixel_format, multisampled);
    }

    void set_null_depth_stencil() { depth_target.sve.init_as_null(); }
};

PR_DEFINE_CMD(end_render_pass){
    // NOTE: Anything useful to pass here?
};

PR_DEFINE_CMD(transition_resources)
{
    struct transition_info
    {
        handle::resource resource;
        resource_state target_state;
    };

    cmd_vector<transition_info, limits::max_resource_transitions> transitions;

public:
    // convenience

    void add(handle::resource res, resource_state target) { transitions.push_back(transition_info{res, target}); }
};

PR_DEFINE_CMD(transition_image_slices)
{
    // Image slice transitions are entirely explicit, and require the user to synchronize before/after resource states

    struct slice_transition_info
    {
        handle::resource resource;
        resource_state source_state;
        resource_state target_state;
        int mip_level;
        int array_slice;
    };

    cmd_vector<slice_transition_info, limits::max_resource_transitions> transitions;

public:
    // convenience

    void add(handle::resource res, resource_state source, resource_state target, int level, int slice)
    {
        transitions.push_back(slice_transition_info{res, source, target, level, slice});
    }
};


PR_DEFINE_CMD(draw)
{
    cmd_vector<shader_argument, limits::max_shader_arguments> shader_arguments;
    handle::pipeline_state pipeline_state;
    handle::resource vertex_buffer;
    handle::resource index_buffer;
    unsigned num_indices;

public:
    // convenience

    void init(handle::pipeline_state pso, unsigned num, handle::resource vb = handle::null_resource, handle::resource ib = handle::null_resource)
    {
        pipeline_state = pso;
        num_indices = num;
        vertex_buffer = vb;
        index_buffer = ib;
    }

    void add_shader_arg(handle::resource cbv, unsigned cbv_off = 0, handle::shader_view sv = handle::null_shader_view)
    {
        shader_arguments.push_back(shader_argument{cbv, sv, cbv_off});
    }
};

PR_DEFINE_CMD(dispatch)
{
    handle::pipeline_state pipeline_state;
    cmd_vector<shader_argument, limits::max_shader_arguments> shader_arguments;
    unsigned dispatch_x;
    unsigned dispatch_y;
    unsigned dispatch_z;

public:
    // convenience

    void init(handle::pipeline_state pso, unsigned x, unsigned y, unsigned z)
    {
        pipeline_state = pso;
        dispatch_x = x;
        dispatch_y = y;
        dispatch_z = z;
    }

    void add_shader_arg(handle::resource cbv, unsigned cbv_off = 0, handle::shader_view sv = handle::null_shader_view)
    {
        shader_arguments.push_back(shader_argument{cbv, sv, cbv_off});
    }
};

PR_DEFINE_CMD(copy_buffer)
{
    handle::resource destination;
    handle::resource source;
    size_t dest_offset;
    size_t source_offset;
    size_t size;

public:
    // convenience

    copy_buffer() = default;
    copy_buffer(handle::resource dest, size_t dest_offset, handle::resource src, size_t src_offset, size_t size)
      : destination(dest), source(src), dest_offset(dest_offset), source_offset(src_offset), size(size)
    {
    }
};

PR_DEFINE_CMD(copy_buffer_to_texture)
{
    handle::resource destination;
    handle::resource source;
    size_t source_offset;
    unsigned dest_width;       ///< width of the destination texture (in the specified MIP map and array element)
    unsigned dest_height;      ///< height of the destination texture (in the specified MIP map and array element)
    unsigned dest_mip_size;    ///< amount of MIP maps in the destination texture
    unsigned dest_mip_index;   ///< index of the MIP map to copy
    unsigned dest_array_index; ///< index of the array element to copy (usually: 0)
    format texture_format;
};

#undef PR_DEFINE_CMD

namespace detail
{
#define PR_X(_val_)                                                                                                                       \
    static_assert(std::is_trivially_copyable_v<::pr::backend::cmd::_val_> && std::is_trivially_destructible_v<::pr::backend::cmd::_val_>, \
                  #_val_ " is not trivially copyable / destructible");
PR_CMD_TYPE_VALUES
#undef PR_X

    /// returns the size in bytes of the given command
    [[nodiscard]] inline constexpr size_t
    get_command_size(detail::cmd_type type)
{
    switch (type)
    {
#define PR_X(_val_)               \
    case detail::cmd_type::_val_: \
        return sizeof(::pr::backend::cmd::_val_);
        PR_CMD_TYPE_VALUES
#undef PR_X
    }
    return 0; // suppress warnings
}

/// returns a string literal corresponding to the command type
[[nodiscard]] inline constexpr char const* to_string(detail::cmd_type type)
{
    switch (type)
    {
#define PR_X(_val_)               \
    case detail::cmd_type::_val_: \
        return #_val_;
        PR_CMD_TYPE_VALUES
#undef PR_X
    }
    return ""; // suppress warnings
}

/// calls F::execute() with the apropriately downcasted command object as a const&
/// (F should have an execute method with overloads for all command objects)
template <class F>
void dynamic_dispatch(detail::cmd_base const& base, F& callback)
{
    switch (base.type)
    {
#define PR_X(_val_)                                                            \
    case detail::cmd_type::_val_:                                              \
        callback.execute(static_cast<::pr::backend::cmd::_val_ const&>(base)); \
        break;
        PR_CMD_TYPE_VALUES
#undef PR_X
    }
}

[[nodiscard]] inline constexpr size_t compute_max_command_size()
{
    size_t res = 0;
#define PR_X(_val_) res = cc::max(res, sizeof(::pr::backend::cmd::_val_));
    PR_CMD_TYPE_VALUES
#undef PR_X
    return res;
}

inline constexpr size_t max_command_size = compute_max_command_size();
}

#undef PR_CMD_TYPE_VALUES
}

struct command_stream_parser
{
public:
    struct iterator_end
    {
    };

    struct iterator
    {
        iterator(std::byte* pos, size_t size)
          : _pos(reinterpret_cast<cmd::detail::cmd_base*>(pos)), _remaining_size(_pos == nullptr ? 0 : static_cast<int64_t>(size))
        {
        }

        bool operator!=(iterator_end) const noexcept { return _remaining_size > 0; }

        iterator& operator++()
        {
            auto const advance = cmd::detail::get_command_size(_pos->type);
            _pos = reinterpret_cast<cmd::detail::cmd_base*>(reinterpret_cast<std::byte*>(_pos) + advance);
            _remaining_size -= advance;
            return *this;
        }

        cmd::detail::cmd_base& operator*() const { return *(_pos); }

    private:
        cmd::detail::cmd_base* _pos = nullptr;
        int64_t _remaining_size = 0;
    };

public:
    command_stream_parser() = default;
    command_stream_parser(std::byte* buffer, size_t size) : _in_buffer(buffer), _size(buffer == nullptr ? 0 : size) {}

    void set_buffer(std::byte* buffer, size_t size)
    {
        _in_buffer = buffer;
        _size = (buffer == nullptr ? 0 : size);
    }

    iterator begin() const { return iterator(_in_buffer, _size); }
    iterator_end end() const { return iterator_end(); }

private:
    std::byte* _in_buffer = nullptr;
    size_t _size = 0;
};

struct command_stream_writer
{
public:
    command_stream_writer() = default;
    command_stream_writer(std::byte* buffer, size_t size) : _out_buffer(buffer), _max_size(size), _cursor(0) {}

    void initialize(std::byte* buffer, size_t size)
    {
        _out_buffer = buffer;
        _max_size = size;
        _cursor = 0;
    }

    void reset() { _cursor = 0; }

    template <class CMDT>
    void add_command(CMDT const& command)
    {
        static_assert(std::is_base_of_v<cmd::detail::cmd_base, CMDT>, "not a command");
        static_assert(std::is_trivially_copyable_v<CMDT>, "command not trivially copyable");
        CC_ASSERT(can_accomodate_t<CMDT>() && "command_stream_writer full");
        new (cc::placement_new, _out_buffer + _cursor) CMDT(command);
        _cursor += sizeof(CMDT);
    }

public:
    size_t size() const { return _cursor; }
    std::byte* buffer() const { return _out_buffer; }

    bool empty() const { return _cursor == 0; }

    int remaining_bytes() const { return static_cast<int>(_max_size) - static_cast<int>(_cursor); }

    template <class CMDT>
    bool can_accomodate_t() const
    {
        static_assert(std::is_base_of_v<cmd::detail::cmd_base, CMDT>, "not a command");
        return static_cast<int>(sizeof(CMDT)) <= remaining_bytes();
    }

    bool can_accomodate(size_t size) const { return static_cast<int>(size) <= remaining_bytes(); }

private:
    std::byte* _out_buffer = nullptr;
    size_t _max_size = 0;
    size_t _cursor = 0;
};
}
