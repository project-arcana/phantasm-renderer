#include "verify.hh"
#ifdef PR_BACKEND_D3D12

#include <cstdio>
#include <cstdlib>

#include <clean-core/assert.hh>

namespace
{
#define CASE_STRINGIFY_RETURN(_val_) \
    case _val_:                      \
        return #_val_
#define CASE_STRINGIFY_ASSIGN(_target_, _val_) \
    case _val_:                                \
        _target_ = #_val_;                     \
        break

char const* get_device_error_literal(HRESULT hr)
{
    switch (hr)
    {
        CASE_STRINGIFY_RETURN(DXGI_ERROR_DEVICE_HUNG);
        CASE_STRINGIFY_RETURN(DXGI_ERROR_DEVICE_REMOVED);
        CASE_STRINGIFY_RETURN(DXGI_ERROR_DEVICE_RESET);
        CASE_STRINGIFY_RETURN(DXGI_ERROR_DRIVER_INTERNAL_ERROR);
        CASE_STRINGIFY_RETURN(DXGI_ERROR_INVALID_CALL);
    }
    return "Unknown Device Error";
}

std::string get_error_string(HRESULT hr, ID3D12Device* device)
{
    std::string res = "";
    switch (hr)
    {
        CASE_STRINGIFY_ASSIGN(res, S_OK);
        CASE_STRINGIFY_ASSIGN(res, D3D11_ERROR_FILE_NOT_FOUND);
        CASE_STRINGIFY_ASSIGN(res, D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS);
        CASE_STRINGIFY_ASSIGN(res, E_FAIL);
        CASE_STRINGIFY_ASSIGN(res, E_INVALIDARG);
        CASE_STRINGIFY_ASSIGN(res, E_OUTOFMEMORY);
        CASE_STRINGIFY_ASSIGN(res, DXGI_ERROR_INVALID_CALL);
        CASE_STRINGIFY_ASSIGN(res, E_NOINTERFACE);
        CASE_STRINGIFY_ASSIGN(res, DXGI_ERROR_DEVICE_REMOVED);
    default:
        res = std::to_string(static_cast<int>(hr));
    }

    if (hr == DXGI_ERROR_DEVICE_REMOVED && device)
    {
        HRESULT removal_reason = device->GetDeviceRemovedReason();
        res += std::string(", removal reason: ") + get_device_error_literal(removal_reason);
    }

    return res;
}

#undef CASE_STRINGIFY_ASSIGN
#undef CASE_STRINGIFY_RETURN
}


void pr::backend::d3d12::detail::d3d12_verify_failure_handler(HRESULT hr, const char* expression, const char* filename, int line, ID3D12Device* device)
{
    // Make sure this really is a failed HRESULT
    CC_ASSERT(FAILED(hr));

    auto const error_string = get_error_string(hr, device);

    // TODO: Proper logging
    fprintf(stderr, "[pr][backend][d3d12] backend verify on `%s' failed.\n", expression);
    fprintf(stderr, "  error %s\n", error_string.c_str());
    fprintf(stderr, "  file %s:%d\n", filename, line);
    fflush(stderr);

    // TODO: Graceful shutdown
    std::abort();
}

#endif
