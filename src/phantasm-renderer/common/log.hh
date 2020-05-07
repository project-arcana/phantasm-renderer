#pragma once

#include <rich-log/log.hh>

namespace pr
{
constexpr void pr_log(rlog::MessageBuilder& builder) { builder.set_domain(rlog::domain("PR")); }
}
