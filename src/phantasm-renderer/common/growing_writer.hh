#pragma once

#include <clean-core/allocator.hh>

#include <phantasm-hardware-interface/commands.hh>

namespace pr
{
// naive growing writer
struct growing_writer
{
    growing_writer() = default;
    growing_writer(size_t initial_size, cc::allocator* alloc = cc::system_allocator);
    growing_writer(growing_writer&& rhs) noexcept : _writer(rhs._writer), _alloc(rhs._alloc)
    {
        rhs._writer.exchange_buffer(nullptr, 0);
        rhs._alloc = cc::system_allocator;
    }

    growing_writer& operator=(growing_writer&& rhs) noexcept;

    ~growing_writer();

    void reset() { _writer.reset(); }

    template <class CmdT>
    void add_command(CmdT const& cmd)
    {
        accomodate(sizeof(CmdT));
        _writer.add_command(cmd);
    }

    [[nodiscard]] std::byte* write_raw_bytes(size_t amount)
    {
        accomodate(amount);
        auto* const res = _writer.buffer_head();
        _writer.advance_cursor(amount);
        return res;
    }

    size_t size() const { return _writer.size(); }
    std::byte* buffer() const { return _writer.buffer(); }
    std::byte* buffer_head() const { return _writer.buffer_head(); }
    size_t max_size() const { return _writer.max_size(); }
    bool is_empty() const { return _writer.empty(); }

    void accomodate(size_t cmd_size)
    {
        if (!_writer.can_accomodate(cmd_size))
        {
            size_t const new_size = (_writer.max_size() + cmd_size) << 1;
            std::byte* const new_buffer = _alloc->realloc(_writer.buffer(), _writer.max_size(), new_size);
            _writer.exchange_buffer(new_buffer, new_size);
        }
    }

    phi::command_stream_writer& raw_writer() { return _writer; }

private:
    phi::command_stream_writer _writer;
    cc::allocator* _alloc = cc::system_allocator;
};

}
