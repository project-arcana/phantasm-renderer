#pragma once

#include <cstdint>

#include <reflector/introspect.hh>

namespace pr
{
struct conflicting_buffer_field;

/// test a C++ struct for correct HLSL constant buffer alignment
template <class BufferT>
[[nodiscard]] bool test_gpu_buffer_alignment(conflicting_buffer_field* out_first_conflict = nullptr, bool verbose = false);


struct conflicting_buffer_field
{
    enum e_reason
    {
        e_hlsl_misaligned,    // the field aligns differently in HLSL than it does in C++
        e_cpp_wrong_order,    // the field was visited in the wrong order in the introspect() function
        e_too_large,          // the field is larger than 16B and cannot be placed in a HLSL constant buffer
        e_too_small,          // the field is smaller than 4B (could be placed, but would behave incorrectly)
        e_array_elem_padding, // the field is an array with elements that are not a multiple of 16B (which will get padded on GPU)
    };

    char const* name;
    e_reason reason;
    unsigned size;
    unsigned offset_in_cpp;
    unsigned offset_in_hlsl;

    static constexpr char const* get_reason_literal(conflicting_buffer_field::e_reason reason);
};


struct BufferAlignmentVisitor
{
    bool valid = true;
    conflicting_buffer_field first_conflict;

    template <class T>
    void operator()(T const& ref, char const* name)
    {
        auto const offset_cpp = uint32_t(reinterpret_cast<size_t>(&ref));
        performCheck(sizeof(T), offset_cpp, name);
    }

    template <class ElemT, size_t N>
    void operator()(ElemT const (&ref)[N], char const* name)
    {
        auto const offset_cpp = uint32_t(reinterpret_cast<size_t>(&ref));
        performCheckArray(sizeof(ElemT), N, offset_cpp, name);
    }

    void printConflictField();

private:
    void performCheck(unsigned field_size_bytes, unsigned offset_cpp, char const* name);

    void performCheckArray(unsigned elem_size_bytes, unsigned array_size, unsigned offset_cpp, char const* name);

    void testOrderConsistency(unsigned new_offset_cpp, unsigned new_field_size_bytes, char const* new_field_name);

    unsigned head_bytes_hlsl = 0;
    unsigned last_cpp_offset = 0;
};

template <class BufferT>
bool test_gpu_buffer_alignment(conflicting_buffer_field* out_first_conflict, bool verbose)
{
    static_assert(std::is_trivially_copyable_v<BufferT>, "test_gpu_buffer_alignment - buffer is not trivially copyable");

    BufferAlignmentVisitor visitor;
    BufferT* volatile dummy_ptr = nullptr;
    rf::do_introspect(visitor, *dummy_ptr);

    if (visitor.valid)
    {
        return true;
    }
    else
    {
        if (out_first_conflict)
            *out_first_conflict = visitor.first_conflict;

        if (verbose)
            visitor.printConflictField();

        return false;
    }
}

constexpr const char* conflicting_buffer_field::get_reason_literal(conflicting_buffer_field::e_reason reason)
{
    switch (reason)
    {
    case conflicting_buffer_field::e_too_large:
        return "too large";
    case conflicting_buffer_field::e_too_small:
        return "too small";
    case conflicting_buffer_field::e_cpp_wrong_order:
        return "introspect order inconsistent";
    case conflicting_buffer_field::e_hlsl_misaligned:
        return "HLSL misaligned";
    case conflicting_buffer_field::e_array_elem_padding:
        return "array elements not a multiple of 16B";
    }
    return "unknown reason";
}
}
