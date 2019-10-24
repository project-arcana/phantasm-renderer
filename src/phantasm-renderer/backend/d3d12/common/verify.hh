#pragma once
#ifdef PR_BACKEND_D3D12

#include <string>

#include <d3d12.h>

#include <clean-core/macros.hh>

namespace pr::backend::d3d12::detail
{
[[noreturn]] CC_COLD_FUNC CC_DONT_INLINE void d3d12_verify_failure_handler(HRESULT hr, char const* expression, char const* filename, int line, ID3D12Device* device);
}

// TODO: option to disable verify in release builds
// NOTE: possibly merge this somehow with CC_ASSERT

/// Executes the given expression and terminates with a detailed error message if the HRESULT indicates failure
#define PR_D3D12_VERIFY(_expr_)                                                                                       \
    do                                                                                                                \
    {                                                                                                                 \
        ::HRESULT const op_res = (_expr_);                                                                            \
        if (CC_UNLIKELY(FAILED(op_res)))                                                                              \
        {                                                                                                             \
            ::pr::backend::d3d12::detail::d3d12_verify_failure_handler(op_res, #_expr_, __FILE__, __LINE__, nullptr); \
        }                                                                                                             \
    } while (0)

/// Executes the given expression and terminates with a detailed error message if the HRESULT indicates failure
/// Additionaly takes a pointer to the current ID3D12Device for further diagnostics
#define PR_D3D12_VERIFY_FULL(_expr_, _device_ptr_)                                                                         \
    do                                                                                                                     \
    {                                                                                                                      \
        ::HRESULT const op_res = (_expr_);                                                                                 \
        if (CC_UNLIKELY(FAILED(op_res)))                                                                                   \
        {                                                                                                                  \
            ::pr::backend::d3d12::detail::d3d12_verify_failure_handler(op_res, #_expr_, __FILE__, __LINE__, _device_ptr_); \
        }                                                                                                                  \
    } while (0)


#endif
