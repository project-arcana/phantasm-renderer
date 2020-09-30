#include "gpu_buffer_alignment.hh"

#include <clean-core/bits.hh>

#include <phantasm-hardware-interface/util.hh>

#include <phantasm-renderer/common/log.hh>

void pr::BufferAlignmentVisitor::printConflictField()
{
    PR_LOG_WARN("struct unsuitable for GPU usage, conflicting field: {}, reason: {}, size: {}, offset: C++: {}, HLSL: {}", first_conflict.name,
                pr::conflicting_buffer_field::get_reason_literal(first_conflict.reason), first_conflict.size, first_conflict.offset_in_cpp,
                first_conflict.offset_in_hlsl);
}

void pr::BufferAlignmentVisitor::performCheck(unsigned field_size_bytes, unsigned offset_cpp, const char* name)
{
    testOrderConsistency(offset_cpp, field_size_bytes, name);

    // no further checks, but C++ ordering is highest priority
    if (!valid)
        return;

    // fields that are too small cannot be placed correctly
    if (field_size_bytes < 4)
    {
        valid = false;
        first_conflict.name = name;
        first_conflict.reason = conflicting_buffer_field::e_too_small;
        first_conflict.size = field_size_bytes;
        first_conflict.offset_in_cpp = 0;
        first_conflict.offset_in_hlsl = 0;
        return;
    }

    auto f_add_field = [this, &name](unsigned offset_cpp, unsigned size_bytes) -> bool {
        // get offset in HLSL
        unsigned const offset_hlsl = phi::util::get_hlsl_constant_buffer_offset(head_bytes_hlsl, size_bytes);

        // error: HLSL misalignment
        if (offset_hlsl != offset_cpp)
        {
            valid = false;
            first_conflict.name = name;
            first_conflict.reason = conflicting_buffer_field::e_hlsl_misaligned;
            first_conflict.size = size_bytes;
            first_conflict.offset_in_cpp = offset_cpp;
            first_conflict.offset_in_hlsl = offset_hlsl;
            return false;
        }

        head_bytes_hlsl = offset_hlsl + size_bytes;
        return true;
    };

    // split field into 16 byte chunks (float4)
    while (field_size_bytes > 16)
    {
        if (!f_add_field(offset_cpp, 16))
            return;

        field_size_bytes -= 16;
        offset_cpp += 16;
    }

    // epilogue / non-oversized field
    f_add_field(offset_cpp, field_size_bytes);
}

void pr::BufferAlignmentVisitor::performCheckArray(unsigned elem_size_bytes, unsigned array_size, unsigned offset_cpp, const char* name)
{
    testOrderConsistency(offset_cpp, elem_size_bytes * array_size, name);

    // no further checks, but C++ ordering is highest priority
    if (!valid)
        return;

    // array elements smaller than a multiple of 16B are up-padded on GPU, this C array will inevitably misalign
    if (cc::mod_pow2(elem_size_bytes, 16u) != 0)
    {
        valid = false;
        first_conflict.name = name;
        first_conflict.reason = conflicting_buffer_field::e_array_elem_padding;
        first_conflict.size = elem_size_bytes;
        first_conflict.offset_in_cpp = 0;
        first_conflict.offset_in_hlsl = 0;
        return;
    }

    // get offset in HLSL for the array start
    unsigned const array_start_offset_hlsl = phi::util::get_hlsl_constant_buffer_offset(head_bytes_hlsl, elem_size_bytes);

    // error: HLSL misalignment
    if (array_start_offset_hlsl != offset_cpp)
    {
        valid = false;
        first_conflict.name = name;
        first_conflict.reason = conflicting_buffer_field::e_hlsl_misaligned;
        first_conflict.size = elem_size_bytes;
        first_conflict.offset_in_cpp = offset_cpp;
        first_conflict.offset_in_hlsl = array_start_offset_hlsl;
        return;
    }

    // this can be a simple multiply since elem_size_bytes % 16 == 0
    // adding array_size float4's to the buffer
    head_bytes_hlsl = array_start_offset_hlsl + elem_size_bytes * array_size;
}

void pr::BufferAlignmentVisitor::testOrderConsistency(unsigned new_offset_cpp, unsigned new_field_size_bytes, const char* new_field_name)
{
    // error: cpp order inconsistent
    if (new_offset_cpp < last_cpp_offset)
    {
        // can be argued to always be a programmer error, not a data error
        // CC_ASSERT(false && "C++ offsets inconsistent, are the calls in introspect ordered correctly?");
        valid = false;

        if (first_conflict.reason != conflicting_buffer_field::e_cpp_wrong_order)
        {
            first_conflict.name = new_field_name;
            first_conflict.reason = conflicting_buffer_field::e_cpp_wrong_order;
            first_conflict.size = new_field_size_bytes;
            first_conflict.offset_in_cpp = new_offset_cpp;
            first_conflict.offset_in_hlsl = 0;
        }
    }

    last_cpp_offset = new_offset_cpp;
}
