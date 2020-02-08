#include "growing_writer.hh"

#include <cstdlib>

pr::growing_writer::growing_writer(size_t initial_size) { _writer.initialize(static_cast<std::byte*>(std::malloc(initial_size)), initial_size); }

pr::growing_writer& pr::growing_writer::operator=(pr::growing_writer&& rhs) noexcept
{
    if (this != &rhs)
    {
        if (_writer.buffer() != nullptr)
        {
            std::free(_writer.buffer());
        }

        _writer = rhs._writer;
        rhs._writer.exchange_buffer(nullptr, 0);
    }
    return *this;
}

pr::growing_writer::~growing_writer()
{
    if (_writer.buffer() != nullptr)
    {
        std::free(_writer.buffer());
    }
}

void pr::growing_writer::accomodate(size_t cmd_size)
{
    if (!_writer.can_accomodate(cmd_size))
    {
        size_t const new_size = (_writer.max_size() + cmd_size) << 1;
        std::byte* const new_buffer = static_cast<std::byte*>(std::malloc(new_size));

        std::memcpy(new_buffer, _writer.buffer(), _writer.size());

        std::free(_writer.buffer());
        _writer.exchange_buffer(new_buffer, new_size);
    }
}