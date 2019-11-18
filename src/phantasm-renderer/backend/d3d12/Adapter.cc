#include "Adapter.hh"

#include <clean-core/assert.hh>

#include "adapter_choice_util.hh"
#include "common/d3d12_sanitized.hh"
#include "common/verify.hh"

void pr::backend::d3d12::Adapter::initialize(const backend_config& config)
{
    // Factory init
    {
        shared_com_ptr<IDXGIFactory> temp_factory;
        PR_D3D12_VERIFY(::CreateDXGIFactory(PR_COM_WRITE(temp_factory)));
        PR_D3D12_VERIFY(temp_factory.get_interface(mFactory));
    }

    // Adapter init
    {
        auto const candidates = get_adapter_candidates();
        auto const chosen_adapter_index = config.adapter_preference == adapter_preference::explicit_index
                                              ? config.explicit_adapter_index
                                              : get_preferred_adapter_index(candidates, config.adapter_preference);

        CC_RUNTIME_ASSERT(chosen_adapter_index != uint32_t(-1));

        shared_com_ptr<IDXGIAdapter> temp_adapter;
        mFactory->EnumAdapters(chosen_adapter_index, temp_adapter.override());
        PR_D3D12_VERIFY(temp_adapter.get_interface(mAdapter));
    }

    // Debug layer init
    if (config.validation != validation_level::off)
    {
        shared_com_ptr<ID3D12Debug> debug_controller;
        bool const debug_init_success = SUCCEEDED(::D3D12GetDebugInterface(PR_COM_WRITE(debug_controller)));

        if (debug_init_success)
        {
            debug_controller->EnableDebugLayer();

            if (config.validation == validation_level::on_extended)
            {
                shared_com_ptr<ID3D12Debug3> debug_controller_v3;
                PR_D3D12_VERIFY(debug_controller.get_interface(debug_controller_v3));
                debug_controller_v3->SetEnableGPUBasedValidation(true);
                debug_controller_v3->SetEnableSynchronizedCommandQueueValidation(true);
            }
        }
        else
        {
            // TODO: Log that the D3D12 SDK is missing
            // refer to https://docs.microsoft.com/en-us/windows/uwp/gaming/use-the-directx-runtime-and-visual-studio-graphics-diagnostic-features
        }
    }
}
