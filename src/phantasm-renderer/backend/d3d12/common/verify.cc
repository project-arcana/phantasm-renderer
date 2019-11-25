#include "verify.hh"

#include <cstdio>
#include <cstdlib>
#include <string>

#include <clean-core/assert.hh>

#include "d3d12_sanitized.hh"

namespace
{
#define CASE_STRINGIFY_RETURN(_val_) \
    case _val_:                      \
        return #_val_

char const* get_device_error_literal(HRESULT hr)
{
    switch (hr)
    {
        CASE_STRINGIFY_RETURN(DXGI_ERROR_DEVICE_HUNG);
        CASE_STRINGIFY_RETURN(DXGI_ERROR_DEVICE_REMOVED);
        CASE_STRINGIFY_RETURN(DXGI_ERROR_DEVICE_RESET);
        CASE_STRINGIFY_RETURN(DXGI_ERROR_DRIVER_INTERNAL_ERROR);
        CASE_STRINGIFY_RETURN(DXGI_ERROR_INVALID_CALL);
    default:
        return "Unknown Device Error";
    }
}

char const* get_general_error_literal(HRESULT hr)
{
    switch (hr)
    {
        CASE_STRINGIFY_RETURN(S_OK);
        CASE_STRINGIFY_RETURN(D3D11_ERROR_FILE_NOT_FOUND);
        CASE_STRINGIFY_RETURN(D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS);
        CASE_STRINGIFY_RETURN(E_FAIL);
        CASE_STRINGIFY_RETURN(E_INVALIDARG);
        CASE_STRINGIFY_RETURN(E_OUTOFMEMORY);
        CASE_STRINGIFY_RETURN(DXGI_ERROR_INVALID_CALL);
        CASE_STRINGIFY_RETURN(E_NOINTERFACE);
        CASE_STRINGIFY_RETURN(DXGI_ERROR_DEVICE_REMOVED);
    default:
        return nullptr;
    }
}

#undef CASE_STRINGIFY_RETURN

std::string get_error_string(HRESULT hr, ID3D12Device* device)
{
    std::string res = "";

    auto const general_error_literal = get_general_error_literal(hr);

    if (general_error_literal != nullptr)
        res = general_error_literal;
    else
        res = std::to_string(static_cast<int>(hr));

    if (hr == DXGI_ERROR_DEVICE_REMOVED && device)
    {
        HRESULT removal_reason = device->GetDeviceRemovedReason();
        res += std::string(", removal reason: ") + get_device_error_literal(removal_reason);
    }

    return res;
}
}


void pr::backend::d3d12::detail::d3d12_verify_failure_handler(HRESULT hr, const char* expression, const char* filename, int line, ID3D12Device* device)
{
    // Make sure this really is a failed HRESULT
    CC_ASSERT(FAILED(hr));

    auto const error_string = get_error_string(hr, device);

    // TODO: Proper logging
    fprintf(stderr, "[pr][backend][d3d12] backend verify on `%s' failed.\n", expression);
    fprintf(stderr, "  error: %s\n", error_string.c_str());
    fprintf(stderr, "  file %s:%d\n", filename, line);
    fflush(stderr);

    // TODO: Graceful shutdown
    std::abort();
}

