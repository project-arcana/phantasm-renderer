#include "RootSignature.hh"

#include <clean-core/assert.hh>

#include "common/d3dx12.hh"
#include "common/verify.hh"

pr::backend::d3d12::RootSignature::~RootSignature() { reset(); }

void pr::backend::d3d12::RootSignature::initialize(ID3D12Device& device, const D3D12_ROOT_SIGNATURE_DESC1& description, D3D_ROOT_SIGNATURE_VERSION version)
{
    auto const num_parameters = description.NumParameters;
    D3D12_ROOT_PARAMETER1* const parameters = num_parameters > 0 ? new D3D12_ROOT_PARAMETER1[num_parameters] : nullptr;

    for (auto i = 0u; i < num_parameters; ++i)
    {
        auto const& root_param = description.pParameters[i];
        auto& param = parameters[i];
        param = root_param;

        if (root_param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
        {
            auto const num_ranges = root_param.DescriptorTable.NumDescriptorRanges;
            D3D12_DESCRIPTOR_RANGE1* const ranges = num_ranges > 0 ? new D3D12_DESCRIPTOR_RANGE1[num_ranges] : nullptr;

            ::memcpy(ranges, root_param.DescriptorTable.pDescriptorRanges, sizeof(D3D12_DESCRIPTOR_RANGE1) * num_ranges);

            param.DescriptorTable.NumDescriptorRanges = num_ranges;
            param.DescriptorTable.pDescriptorRanges = ranges;

            // Set the bit mask depending on the type of descriptor table.
            if (num_ranges > 0)
            {
                switch (ranges[0].RangeType)
                {
                case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
                case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
                case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
                    mDescriptorTableBitmask |= (1 << i);
                    break;
                case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
                    mSamplerTableBitmask |= (1 << i);
                    break;
                }
            }

            // Count the number of descriptors in the descriptor table.
            for (auto j = 0u; j < num_ranges; ++j)
            {
                mNumDescriptorsPerTable[i] += ranges[j].NumDescriptors;
            }
        }
    }

    mDescription.NumParameters = num_parameters;
    mDescription.pParameters = parameters;

    auto const num_static_samplers = description.NumStaticSamplers;
    D3D12_STATIC_SAMPLER_DESC* const static_samplers = num_static_samplers > 0 ? new D3D12_STATIC_SAMPLER_DESC[num_static_samplers] : nullptr;

    if (static_samplers)
    {
        ::memcpy(static_samplers, description.pStaticSamplers, sizeof(D3D12_STATIC_SAMPLER_DESC) * num_static_samplers);
    }

    mDescription.NumStaticSamplers = num_static_samplers;
    mDescription.pStaticSamplers = static_samplers;

    mDescription.Flags = description.Flags;

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC versioned_description;
    versioned_description.Init_1_1(num_parameters, parameters, num_static_samplers, static_samplers, description.Flags);

    // Serialize the root signature.
    shared_com_ptr<ID3DBlob> rootSignatureBlob;
    shared_com_ptr<ID3DBlob> errorBlob;
    PR_D3D12_VERIFY(::D3DX12SerializeVersionedRootSignature(&versioned_description, version, rootSignatureBlob.override(), errorBlob.override()));

    // Create the root signature.
    PR_D3D12_VERIFY(device.CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), PR_COM_WRITE(mRootSignature)));
}

void pr::backend::d3d12::RootSignature::reset()
{
    if (!mRootSignature.is_valid())
        return;

    for (auto i = 0u; i < mDescription.NumParameters; ++i)
    {
        auto const& root_param = mDescription.pParameters[i];

        if (root_param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
        {
            delete[] root_param.DescriptorTable.pDescriptorRanges;
        }
    }

    delete[] mDescription.pParameters;
    delete[] mDescription.pStaticSamplers;

    mDescriptorTableBitmask = 0;
    mSamplerTableBitmask = 0;
    cc::fill(mNumDescriptorsPerTable, 0);

    mRootSignature = nullptr;
}
