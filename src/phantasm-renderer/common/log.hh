#pragma once

#include <rich-log/detail/log_impl.hh>

namespace pr::detail
{
static constexpr rlog::domain domain = rlog::domain("PR");

inline void info_log(rlog::MessageBuilder& builder) { builder.set_domain(domain); }
inline void warn_log(rlog::MessageBuilder& builder)
{
    builder.set_domain(domain);
    builder.set_severity(rlog::severity::warning);
}
inline void err_log(rlog::MessageBuilder& builder)
{
    builder.set_domain(domain);
    builder.set_severity(rlog::severity::error);
}
inline void assert_log(rlog::MessageBuilder& builder)
{
    builder.set_domain(domain);
    builder.set_severity(rlog::severity::critical);
}
}

#define PR_LOG RICH_LOG_IMPL(pr::detail::info_log)
#define PR_LOG_WARN RICH_LOG_IMPL(pr::detail::warn_log)
#define PR_LOG_ERROR RICH_LOG_IMPL(pr::detail::err_log)
