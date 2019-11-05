#pragma once

#include <clean-core/array.hh>

#include "common/d3d12_sanitized.hh"

#include "common/shared_com_ptr.hh"

namespace pr::backend::d3d12
{
/// Represents a ID3D12RootSignature
class RootSignature
{
    // reference type
public:
    RootSignature() { cc::fill(mNumDescriptorsPerTable, 0u); }
    ~RootSignature();

    RootSignature(RootSignature const&) = delete;
    RootSignature(RootSignature&&) noexcept = delete;
    RootSignature& operator=(RootSignature const&) = delete;
    RootSignature& operator=(RootSignature&&) noexcept = delete;

    void initialize(ID3D12Device& device, D3D12_ROOT_SIGNATURE_DESC1 const& rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion);

    [[nodiscard]] D3D12_ROOT_SIGNATURE_DESC1 const& getDescription() const { return mDescription; }

    [[nodiscard]] cc::uint32 getNumDescriptors(unsigned root_index) const { return mNumDescriptorsPerTable[root_index]; }

    [[nodiscard]] cc::uint32 getSamplerTableBitmask() const { return mSamplerTableBitmask; }
    [[nodiscard]] cc::uint32 getDescriptorTableBitmask() const { return mDescriptorTableBitmask; }
    [[nodiscard]] cc::uint32 getBitmask(D3D12_DESCRIPTOR_HEAP_TYPE type) const
    {
        switch (type)
        {
        case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
            return getDescriptorTableBitmask();
        case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
            return getSamplerTableBitmask();
        default:
            return 0;
        }
    }

    [[nodiscard]] ID3D12RootSignature& getRootSignature() const { return *mRootSignature.get(); }
    [[nodiscard]] shared_com_ptr<ID3D12RootSignature> getRootSignatureShared() const { return mRootSignature; }

private:
    void reset();

private:
    D3D12_ROOT_SIGNATURE_DESC1 mDescription;
    shared_com_ptr<ID3D12RootSignature> mRootSignature;

    cc::array<cc::uint32, 32> mNumDescriptorsPerTable;

    cc::uint32 mSamplerTableBitmask = 0;
    cc::uint32 mDescriptorTableBitmask = 0;
};
}
