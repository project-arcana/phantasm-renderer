#pragma once

#include <string>

namespace pr::backend::detail
{
[[nodiscard]] std::string formatted_stl_string(char const* format, ...);

}
