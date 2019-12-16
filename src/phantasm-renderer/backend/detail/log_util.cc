#include "log_util.hh"

#include <cstdio>
#include <iostream>

void pr::backend::log::dump_hex(const char* description, const void* data, int length)
{
    int i;
    unsigned char buff[17];                                         // stores the ASCII data
    auto const* const pc = static_cast<unsigned char const*>(data); // cast to make the code cleaner.

    // Output description if given.

    if (description != nullptr)
        std::printf("%s:\n", description);

    // Process every byte in the data.

    for (i = 0; i < length; i++)
    {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0)
        {
            // Just don't print ASCII for the zeroth line.

            if (i != 0)
                std::printf("  %s\n", buff);

            // Output the offset.

            std::printf("  %04x ", i);
        }

        // Now the hex code for the specific character.

        std::printf(" %02x", pc[i]);

        // And store a printable ASCII character for later.

        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.

    while ((i % 16) != 0)
    {
        std::printf("   ");
        i++;
    }

    // And print the final ASCII bit.

    std::printf("  %s\n", buff);
}

void pr::backend::log::print_cmd(const char* prefix, const pr::backend::cmd::dispatch& dispatch)
{
    std::cout << prefix << "cmd::dispatch (" << dispatch.dispatch_x << ", " << dispatch.dispatch_y << ", " << dispatch.dispatch_z << ")" << std::endl;
    log::dump_hex("    root constant data", &dispatch.root_constants[0], sizeof(dispatch.root_constants));
    std::cout << "    pipeline_state: " << dispatch.pipeline_state.index << std::endl;
    for (uint8_t i = 0u; i < dispatch.shader_arguments.size(); ++i)
    {
        auto const& sa = dispatch.shader_arguments[i];
        std::cout << "    arg #" << static_cast<int>(i) << ": shader_view: " << sa.shader_view.index << ", constant_buffer: " << sa.constant_buffer.index
                  << ", constant_buffer_offset: " << sa.constant_buffer_offset << std::endl;
    }
}
