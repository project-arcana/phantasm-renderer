#pragma once

#include <string>
#include <vector>

#include <clean-core/span.hh>

namespace pr::backend::detail
{
/// Creates a vector of char* strings from a contiguous container of std::strings
/// Note that the return value must not outlive the argument
[[nodiscard]] inline std::vector<char const*> to_cstring_array(cc::span<std::string const> container)
{
    std::vector<char const*> res(container.size());

    for (auto i = 0u; i < container.size(); ++i)
    {
        res[i] = container[i].c_str();
    }

    return res;
}

[[nodiscard]] std::string formatted_stl_string(char const* format, ...);

}
