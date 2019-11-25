#pragma once

#include <phantasm-renderer/backend/types.hh>

#include "common/d3d12_fwd.hh"

#include "common/shared_com_ptr.hh"

namespace pr::backend::d3d12
{
/// Represents a IDXGIAdapter, the uppermost object in the D3D12 hierarchy
class Adapter
{
    // reference type
public:
    Adapter() = default;
    Adapter(Adapter const&) = delete;
    Adapter(Adapter&&) noexcept = delete;
    Adapter& operator=(Adapter const&) = delete;
    Adapter& operator=(Adapter&&) noexcept = delete;

    void initialize(backend_config const& config);

    [[nodiscard]] IDXGIAdapter& getAdapter() const { return *mAdapter.get(); }
    [[nodiscard]] shared_com_ptr<IDXGIAdapter> getAdapterShared() const { return mAdapter; }

    [[nodiscard]] IDXGIFactory4& getFactory() const { return *mFactory.get(); }
    [[nodiscard]] shared_com_ptr<IDXGIFactory4> getFactoryShared() const { return mFactory; }

private:
    shared_com_ptr<IDXGIAdapter> mAdapter;
    shared_com_ptr<IDXGIFactory4> mFactory;

private:
};

}
