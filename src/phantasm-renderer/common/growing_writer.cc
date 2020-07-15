#include "growing_writer.hh"

#include <cstdlib>

pr::growing_writer::growing_writer(size_t initial_size, cc::allocator* alloc) : _alloc(alloc)
{
    CC_CONTRACT(alloc != nullptr);
    _writer.initialize(_alloc->alloc(initial_size), initial_size);
}

pr::growing_writer& pr::growing_writer::operator=(pr::growing_writer&& rhs) noexcept
{
    if (this != &rhs)
    {
        if (_writer.buffer() != nullptr)
        {
            _alloc->free(_writer.buffer());
        }

        _writer = rhs._writer;
        _alloc = rhs._alloc;
        rhs._writer.exchange_buffer(nullptr, 0);
        rhs._alloc = cc::system_allocator;
    }
    return *this;
}

pr::growing_writer::~growing_writer() { _alloc->free(_writer.buffer()); }

void pr::growing_writer::accomodate(size_t cmd_size)
{
    if (!_writer.can_accomodate(cmd_size))
    {
        size_t const new_size = (_writer.max_size() + cmd_size) << 1;
        std::byte* const new_buffer = _alloc->realloc(_writer.buffer(), _writer.max_size(), new_size);
        _writer.exchange_buffer(new_buffer, new_size);
    }
}
