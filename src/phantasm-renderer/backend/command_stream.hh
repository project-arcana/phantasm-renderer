#pragma once

#include <cstring>

#include <clean-core/capped_vector.hh>
#include <clean-core/utility.hh>

#include <typed-geometry/tg-lean.hh>

#include "types.hh"

namespace pr::backend
{
namespace limits
{
inline constexpr auto max_render_targets = 8u;
inline constexpr auto max_resource_transitions = 4u;
inline constexpr auto max_shader_arguments = 4u;
}

namespace cmd
{
namespace detail
{
#define PR_CMD_TYPE_VALUES       \
    PR_X(draw)                   \
    PR_X(transition_resources)   \
    PR_X(copy_buffer)            \
    PR_X(copy_buffer_to_texture) \
    PR_X(begin_render_pass)      \
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

#define PR_DEFINE_CMD(_type_) struct _type_ final : detail::typed_cmd<detail::cmd_type::_type_>

PR_DEFINE_CMD(begin_render_pass)
{
    enum class rt_clear_type : uint8_t
    {
        clear,
        dont_care,
        load
    };

    struct render_target_info
    {
        handle::resource resource;
        tg::color4 clear_value;
        rt_clear_type clear_type;
    };

    struct depth_stencil_info
    {
        handle::resource resource;
        float clear_value_depth;
        uint8_t clear_value_stencil;
        rt_clear_type clear_type;
    };

    cc::capped_vector<render_target_info, limits::max_render_targets> render_targets;
    cc::capped_vector<depth_stencil_info, 1> depth_target;
    tg::ivec2 viewport;
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

    cc::capped_vector<transition_info, limits::max_resource_transitions> transitions;
};

PR_DEFINE_CMD(draw)
{
    handle::pipeline_state pipeline_state;
    handle::resource vertex_buffer;
    handle::resource index_buffer;
    unsigned num_indices;
    cc::capped_vector<shader_argument, limits::max_shader_arguments> shader_arguments;
};

PR_DEFINE_CMD(copy_buffer)
{
    handle::resource destination;
    handle::resource source;
    size_t dest_offset;
    size_t source_offset;
    size_t size;

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
    unsigned mip_width;
    unsigned mip_height;
    unsigned subresource_index;
    format texture_format;
};

#undef PR_DEFINE_CMD

namespace detail
{
/// returns the size in bytes of the given command
[[nodiscard]] inline constexpr size_t get_command_size(detail::cmd_type type)
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
    command_stream_writer(std::byte* buffer, size_t size) : _out_buffer(buffer), _max_size(size) {}

    template <class CMDT>
    void add_command(CMDT const& command)
    {
        static_assert(std::is_base_of_v<cmd::detail::cmd_base, CMDT>, "not a command");
        CC_ASSERT(static_cast<int>(sizeof(CMDT)) <= remaining_bytes());
        std::memcpy(_out_buffer + _cursor, &command, sizeof(CMDT));
        _cursor += sizeof(CMDT);
    }

    size_t size() const { return _cursor; }
    std::byte* buffer() const { return _out_buffer; }

    int remaining_bytes() const { return static_cast<int>(_max_size) - static_cast<int>(_cursor); }

private:
    std::byte* _out_buffer = nullptr;
    size_t _max_size = 0;
    size_t _cursor = 0;
};
}
