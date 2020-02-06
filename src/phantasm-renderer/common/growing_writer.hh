#pragma once

#include <phantasm-hardware-interface/commands.hh>

namespace pr
{
struct growing_writer
{
    growing_writer(size_t initial_size);
    growing_writer(growing_writer&& rhs) noexcept : _writer(rhs._writer) { rhs._writer.exchange_buffer(nullptr, 0); }
    growing_writer& operator=(growing_writer&& rhs) noexcept;

    ~growing_writer();

    void reset() { _writer.reset(); }

    template <class CmdT>
    void add_command(CmdT const& cmd)
    {
        accomodate(sizeof(CmdT));
        _writer.add_command(cmd);
    }

    size_t size() const { return _writer.size(); }
    std::byte* buffer() const { return _writer.buffer(); }
    size_t max_size() const { return _writer.max_size(); }
    bool is_empty() const { return _writer.empty(); }

    void accomodate(size_t cmd_size);

    phi::command_stream_writer& raw_writer() { return _writer; }

private:
    phi::command_stream_writer _writer;
};

}
