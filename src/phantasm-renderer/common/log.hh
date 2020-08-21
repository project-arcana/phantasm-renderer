#pragma once

#include <rich-log/detail/log_impl.hh>

namespace pr::detail
{
static constexpr rlog::domain domain = rlog::domain("PR");
static constexpr rlog::severity assert_severity = rlog::severity("ASSERT", "\u001b[38;5;196m\u001b[1m");

constexpr void info_log(rlog::MessageBuilder& builder) { builder.set_domain(domain); }
constexpr void warn_log(rlog::MessageBuilder& builder)
{
    builder.set_domain(domain);
    builder.set_severity(rlog::severity::warning());
    builder.set_use_error_stream(true);
}
constexpr void err_log(rlog::MessageBuilder& builder)
{
    builder.set_domain(domain);
    builder.set_severity(rlog::severity::error());
    builder.set_use_error_stream(true);
}
constexpr void assert_log(rlog::MessageBuilder& builder)
{
    builder.set_domain(domain);
    builder.set_severity(assert_severity);
    builder.set_use_error_stream(true);
}
}

#define PR_LOG RICH_LOG_IMPL(pr::detail::info_log)
#define PR_LOG_WARN RICH_LOG_IMPL(pr::detail::warn_log)
#define PR_LOG_ERROR RICH_LOG_IMPL(pr::detail::err_log)
