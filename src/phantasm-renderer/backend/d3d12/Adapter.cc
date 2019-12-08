#include "Adapter.hh"

#include <iostream>

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
    bool is_intel_gpu = false;
    {
        // choose the adapter
        auto const candidates = get_adapter_candidates();
        auto const chosen_adapter_index = config.adapter_preference == adapter_preference::explicit_index
                                              ? config.explicit_adapter_index
                                              : candidates[get_preferred_gpu(candidates, config.adapter_preference)].index;

        CC_RUNTIME_ASSERT(chosen_adapter_index != uint32_t(-1));

        // detect intel GPUs for GBV workaround
        for (auto const& candidate : candidates)
        {
            if (candidate.index == chosen_adapter_index)
            {
                is_intel_gpu = candidate.vendor == gpu_vendor::intel;
                break;
            }
        }

        // create the adapter
        shared_com_ptr<IDXGIAdapter> temp_adapter;
        mFactory->EnumAdapters(chosen_adapter_index, temp_adapter.override());
        PR_D3D12_VERIFY(temp_adapter.get_interface(mAdapter));
    }

    // Debug layer init
    if (config.validation != validation_level::off)
    {
        shared_com_ptr<ID3D12Debug> debug_controller;
        bool const debug_init_success = detail::hr_succeeded(::D3D12GetDebugInterface(PR_COM_WRITE(debug_controller)));

        if (debug_init_success && debug_controller.is_valid())
        {
            debug_controller->EnableDebugLayer();

            if (config.validation >= validation_level::on_extended)
            {
                if (is_intel_gpu)
                {
                    std::cout << "[pr][backend][d3d12] info: GPU-based validation requested on an Intel GPU, disabling due to known crashes" << std::endl;
                }
                else
                {
                    shared_com_ptr<ID3D12Debug3> debug_controller_v3;
                    bool const gbv_init_success = detail::hr_succeeded(debug_controller.get_interface(debug_controller_v3));

                    if (gbv_init_success && debug_controller_v3.is_valid())
                    {
                        // TODO: even if this succeeded, we could have still
                        // launched from inside NSight, where SetEnableSynchronizedCommandQueueValidation
                        // will crash
                        debug_controller_v3->SetEnableGPUBasedValidation(true);
                        debug_controller_v3->SetEnableSynchronizedCommandQueueValidation(true);
                    }
                    else
                    {
                        std::cerr << "[pr][backend[d3d12] warning: failed to enable GPU-based validation" << std::endl;
                    }
                }
            }
        }
        else
        {
            std::cerr << "[pr][backend[d3d12] warning: failed to enable validation" << std::endl;
            std::cerr << "  verify that the D3D12 SDK is installed on this machine" << std::endl;
            std::cerr << "  refer to "
                         "https://docs.microsoft.com/en-us/windows/uwp/gaming/use-the-directx-runtime-and-visual-studio-graphics-diagnostic-features"
                      << std::endl;
        }
    }
}
