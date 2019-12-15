#include "verify.hh"

#include <cstdio>
#include <cstdlib>

#include <iostream>

#include <clean-core/assert.hh>

#include "d3d12_sanitized.hh"
#include "shared_com_ptr.hh"

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
        return "Unknown HRESULT";
    }
}

#undef CASE_STRINGIFY_RETURN

void print_dred_information(ID3D12Device* device)
{
    HRESULT removal_reason = device->GetDeviceRemovedReason();
    std::cerr << "[pr][backend][d3d12][DRED] device removal reason: " << get_device_error_literal(removal_reason) << std::endl;

    pr::backend::d3d12::shared_com_ptr<ID3D12DeviceRemovedExtendedData> dred;
    if (SUCCEEDED(device->QueryInterface(PR_COM_WRITE(dred))))
    {
        std::cerr << "[pr][backend][d3d12][DRED] DRED detected, querying outputs" << std::endl;
        D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT DredAutoBreadcrumbsOutput;
        D3D12_DRED_PAGE_FAULT_OUTPUT DredPageFaultOutput;
        auto hr1 = dred->GetAutoBreadcrumbsOutput(&DredAutoBreadcrumbsOutput);
        auto hr2 = dred->GetPageFaultAllocationOutput(&DredPageFaultOutput);

        //::DebugBreak();

        if (SUCCEEDED(hr1))
        {
            // TODO: Breadcrumb output
            D3D12_AUTO_BREADCRUMB_NODE const* breadcrumb = DredAutoBreadcrumbsOutput.pHeadAutoBreadcrumbNode;
            auto num_breadcrumbs = 0u;
            while (breadcrumb != nullptr && num_breadcrumbs < 10)
            {
                std::wcerr << "[pr][backend][d3d12][DRED] bc #" << num_breadcrumbs << " size " << breadcrumb->BreadcrumbCount<< std::endl;

                if (breadcrumb->pCommandListDebugNameA != nullptr)
                    std::wcerr << "[pr][backend][d3d12][DRED]   on list \"" << breadcrumb->pCommandListDebugNameA << "\"" << std::endl;
                if (breadcrumb->pCommandQueueDebugNameA != nullptr)
                    std::wcerr << "[pr][backend][d3d12][DRED]   on queue \"" << breadcrumb->pCommandQueueDebugNameA << "\"" << std::endl;

                std::wcerr << "[pr][backend][d3d12][DRED]     ";
                unsigned const last_executed_i = *breadcrumb->pLastBreadcrumbValue;
                for (auto i = 0u; i < breadcrumb->BreadcrumbCount; ++i)
                {
                    if (i == last_executed_i)
                        std::wcerr << "[[-  " << breadcrumb->pCommandHistory[i] << " -]]";
                    else
                        std::wcerr << "[" << breadcrumb->pCommandHistory[i] << "]";
                }
                if (last_executed_i == breadcrumb->BreadcrumbCount)
                    std::wcerr << "  (fully executed)";
                else
                    std::wcerr << "  (last executed: " << last_executed_i << ")";

                std::wcerr << std::endl;

                breadcrumb = breadcrumb->pNext;
                ++num_breadcrumbs;
            }
            std::cerr << "[pr][backend][d3d12][DRED] end of breadcrumb data" << std::endl;
        }
        else
        {
            std::cerr << "[pr][backend][d3d12][DRED] DRED breadcrumb output query failed" << std::endl;
            std::cerr << "[pr][backend][d3d12][DRED] use validation_level::on_extended_dred" << std::endl;
        }

        if (SUCCEEDED(hr2))
        {
            std::cerr << "[pr][backend][d3d12][DRED] page fault VA: " << DredPageFaultOutput.PageFaultVA << std::endl;
            D3D12_DRED_ALLOCATION_NODE const* freed_node = DredPageFaultOutput.pHeadRecentFreedAllocationNode;
            while (freed_node != nullptr)
            {
                if (freed_node->ObjectNameA)
                    std::wcerr << "[pr][backend][d3d12][DRED] recently freed: " << freed_node->ObjectNameA << std::endl;
                freed_node = freed_node->pNext;
            }

            D3D12_DRED_ALLOCATION_NODE const* allocated_node = DredPageFaultOutput.pHeadExistingAllocationNode;
            while (allocated_node != nullptr)
            {
                if (allocated_node->ObjectNameA)
                    std::wcerr << "[pr][backend][d3d12][DRED] allocated: " << allocated_node->ObjectNameA << std::endl;
                allocated_node = allocated_node->pNext;
            }

            std::cerr << "[pr][backend][d3d12][DRED] end of page fault data" << std::endl;
        }
        else
        {
            std::cerr << "[pr][backend][d3d12][DRED] DRED pagefault output query failed" << std::endl;
            std::cerr << "[pr][backend][d3d12][DRED] use validation_level::on_extended_dred" << std::endl;
        }
    }
    else
    {
        std::cerr << "[pr][backend][d3d12][DRED] no DRED active, use validation_level::on_extended_dred" << std::endl;
    }
}
}


void pr::backend::d3d12::detail::verify_failure_handler(HRESULT hr, const char* expression, const char* filename, int line, ID3D12Device* device)
{
    // Make sure this really is a failed HRESULT
    CC_RUNTIME_ASSERT(FAILED(hr));

    // TODO: Proper logging
    fprintf(stderr, "[pr][backend][d3d12] backend verify on `%s' failed.\n", expression);
    fprintf(stderr, "  error: %s\n", get_general_error_literal(hr));
    fprintf(stderr, "  file %s:%d\n", filename, line);
    fflush(stderr);

    if (hr == DXGI_ERROR_DEVICE_REMOVED && device)
    {
        print_dred_information(device);
    }

    // TODO: Graceful shutdown
    std::abort();
}

void pr::backend::d3d12::detail::dred_assert_handler(void* device_child, const char* expression, const char* filename, int line)
{
    fprintf(stderr, "[pr][backend][d3d12] DRED assert on `%s' failed.\n", expression);
    fprintf(stderr, "  file %s:%d\n", filename, line);
    fflush(stderr);

    auto* const as_device_child = static_cast<ID3D12DeviceChild*>(device_child);

    shared_com_ptr<ID3D12Device> recovered_device;
    auto const hr = as_device_child->GetDevice(PR_COM_WRITE(recovered_device));
    if (hr_succeeded(hr) && recovered_device.is_valid())
    {
        print_dred_information(recovered_device);
    }
    else
    {
        std::cerr << "[pr][backend][d3d12] Failed to recover device from ID3D12DeviceChild " << device_child << std::endl;
    }

    // TODO: Graceful shutdown
    std::abort();
}
