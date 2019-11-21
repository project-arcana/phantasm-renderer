#pragma once

#include <cstring>

#include <clean-core/capped_vector.hh>

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
#define PR_CMD_TYPE_VALUES     \
    PR_X(draw)                 \
    PR_X(transition_resources) \
    PR_X(begin_render_pass)    \
    PR_X(end_render_pass)      \
    PR_X(final_command)

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
    static constexpr cmd_type s_type = TYPE;
    static constexpr cmd_queue_type s_queue_type = QUEUE;
    typed_cmd() : cmd_base(TYPE) {}
};

}

#define PR_DEFINE_CMD(_type_) struct _type_ : detail::typed_cmd<detail::cmd_type::_type_>

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
};

PR_DEFINE_CMD(end_render_pass){};

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

PR_DEFINE_CMD(final_command){
    // signals the end of the stream
};

#undef PR_DEFINE_CMD

namespace detail
{
[[nodiscard]] inline constexpr size_t get_command_size(detail::cmd_type type)
{
    switch (type)
    {
#define PR_X(_val_)               \
    case detail::cmd_type::_val_: \
        return sizeof(_val_);
        PR_CMD_TYPE_VALUES
#undef PR_X
    }
}

[[nodiscard]] inline constexpr char const* command_type_to_string(detail::cmd_type type)
{
    switch (type)
    {
#define PR_X(_val_)               \
    case detail::cmd_type::_val_: \
        return #_val_;
        PR_CMD_TYPE_VALUES
#undef PR_X
    }
}

template <class F>
void reinterpret_dispatch(detail::cmd_base const& base, F& callback)
{
    switch (base.type)
    {
#define PR_X(_val_)                                        \
    case detail::cmd_type::_val_:                          \
        callback.execute(static_cast<_val_ const&>(base)); \
        break;
        PR_CMD_TYPE_VALUES
#undef PR_X
    }
}
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
        iterator(char* pos) : _pos(reinterpret_cast<cmd::detail::cmd_base*>(pos)) {}

        bool operator==(iterator_end) const noexcept { return _pos->type == cmd::detail::cmd_type::final_command; }
        bool operator!=(iterator_end) const noexcept { return _pos->type != cmd::detail::cmd_type::final_command; }

        //        bool operator==(iterator other) const noexcept { return other._pos == _pos; }
        //        bool operator!=(iterator other) const noexcept { return other._pos != _pos; }
        //        bool operator<(iterator other) const noexcept { return _pos < other._pos; }
        //        bool operator<=(iterator other) const noexcept { return _pos <= other._pos; }
        //        bool operator>(iterator other) const noexcept { return _pos > other._pos; }
        //        bool operator>=(iterator other) const noexcept { return _pos >= other._pos; }

        iterator& operator++()
        {
            _pos = reinterpret_cast<cmd::detail::cmd_base*>(reinterpret_cast<char*>(_pos) + cmd::detail::get_command_size(_pos->type));
            return *this;
        }

        cmd::detail::cmd_base& operator*() const { return *(_pos); }

    private:
        cmd::detail::cmd_base* _pos = nullptr;
    };

public:
    command_stream_parser(char* buffer) : _input_stream(buffer) {}

    iterator begin() const { return iterator(_input_stream); }
    iterator_end end() const { return iterator_end(); }

private:
    char* _input_stream = nullptr;
};

struct command_stream_writer
{
public:
    command_stream_writer(char* buffer, size_t size) : _output_stream(buffer), _max_size(size) {}

    template <class CMDT>
    void add_command(CMDT const& command)
    {
        static_assert(std::is_base_of_v<cmd::detail::cmd_base, CMDT>, "Type is not a command");
        CC_ASSERT(sizeof(CMDT) <= _max_size - _cursor);
        std::memcpy(_output_stream + _cursor, &command, sizeof(CMDT));
        _cursor += sizeof(CMDT);
    }

    void finalize() { add_command(cmd::final_command{}); }


private:
    char* _output_stream = nullptr;
    size_t _max_size = 0;
    size_t _cursor = 0;
};
}
