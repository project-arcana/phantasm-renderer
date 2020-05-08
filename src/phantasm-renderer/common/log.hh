#pragma once

#include <rich-log/log.hh>

namespace pr
{
constexpr void logger(rlog::MessageBuilder& builder) { builder.set_domain(rlog::domain("PR")); }
}
