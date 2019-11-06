#pragma once
#ifdef PR_BACKEND_D3D12

#include <clean-core/macros.hh>

#include <phantasm-renderer/backend/detail/value_category.hh>

struct ID3D12Device;
typedef long HRESULT;

namespace pr::backend::d3d12::detail
{
[[noreturn]] CC_COLD_FUNC CC_DONT_INLINE void d3d12_verify_failure_handler(HRESULT hr, char const* expression, char const* filename, int line, ID3D12Device* device);
[[nodiscard]] inline constexpr bool hr_failed(HRESULT hr) { return hr < 0; }
[[nodiscard]] inline constexpr bool hr_succeeded(HRESULT hr) { return hr >= 0; }
}

// TODO: option to disable verify in release builds
// NOTE: possibly merge this somehow with CC_ASSERT

/// Terminates with a detailed error message if the given HRESULT lvalue indicates failure
#define PR_D3D12_ASSERT(_val_)                                                                                                 \
    static_assert(PR_IS_LVALUE_EXPRESSION(_val_), "Use PR_D3D12_VERIFY for prvalue expressions");                              \
    (CC_UNLIKELY(::pr::backend::d3d12::detail::hr_failed(_val_))                                                               \
         ? ::pr::backend::d3d12::detail::d3d12_verify_failure_handler(_val_, #_val_ " is failed", __FILE__, __LINE__, nullptr) \
         : void(0))


/// Terminates with a detailed error message if the given HRESULT lvalue indicates failure, takes the current ID3D12 Device for further information
#define PR_D3D12_ASSERT_FULL(_val_, _device_ptr_)                                                                                   \
    static_assert(PR_IS_LVALUE_EXPRESSION(_val_), "Use PR_D3D12_VERIFY_FULL for prvalue expressions");                              \
    (CC_UNLIKELY(::pr::backend::d3d12::detail::hr_failed(_val_))                                                                    \
         ? ::pr::backend::d3d12::detail::d3d12_verify_failure_handler(_val_, #_val_ " is failed", __FILE__, __LINE__, _device_ptr_) \
         : void(0))

/// Executes the given expression and terminates with a detailed error message if the HRESULT indicates failure
#define PR_D3D12_VERIFY(_expr_)                                                                                       \
    static_assert(PR_IS_PRVALUE_EXPRESSION(_expr_), "Use PR_D3D12_ASSERT for lvalue expressions");                    \
    do                                                                                                                \
    {                                                                                                                 \
        ::HRESULT const op_res = (_expr_);                                                                            \
        if (CC_UNLIKELY(::pr::backend::d3d12::detail::hr_failed(op_res)))                                             \
        {                                                                                                             \
            ::pr::backend::d3d12::detail::d3d12_verify_failure_handler(op_res, #_expr_, __FILE__, __LINE__, nullptr); \
        }                                                                                                             \
    } while (0)

/// Executes the given expression and terminates with a detailed error message if the HRESULT indicates failure, takes the current ID3D12 Device for further information
#define PR_D3D12_VERIFY_FULL(_expr_, _device_ptr_)                                                                         \
    static_assert(PR_IS_PRVALUE_EXPRESSION(_expr_), "Use PR_D3D12_VERIFY_FULL for lvalue expressions");                    \
    do                                                                                                                     \
    {                                                                                                                      \
        ::HRESULT const op_res = (_expr_);                                                                                 \
        if (CC_UNLIKELY(::pr::backend::d3d12::detail::hr_failed(op_res)))                                                  \
        {                                                                                                                  \
            ::pr::backend::d3d12::detail::d3d12_verify_failure_handler(op_res, #_expr_, __FILE__, __LINE__, _device_ptr_); \
        }                                                                                                                  \
    } while (0)


#endif
