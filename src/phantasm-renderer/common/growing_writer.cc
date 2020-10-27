#include "growing_writer.hh"

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


