#include "root_sig_cache.hh"

void pr::backend::d3d12::RootSignatureCache::initialize(unsigned size_estimate) { mCache.reserve(size_estimate); }

void pr::backend::d3d12::RootSignatureCache::destroy() { reset(); }

pr::backend::d3d12::root_signature* pr::backend::d3d12::RootSignatureCache::getOrCreate(ID3D12Device& device, pr::backend::d3d12::RootSignatureCache::key_t arg_shapes)
{
    if (auto const it = mCache.find(arg_shapes); it != mCache.end())
    {
        return &it->second;
    }
    else
    {
        auto [new_it, success] = mCache.emplace(arg_shapes, root_signature{});
        CC_ASSERT(success);
        initialize_root_signature(new_it->second, device, arg_shapes);
        return &new_it->second;
    }
}

void pr::backend::d3d12::RootSignatureCache::reset()
{
    for (auto const& elem : mCache)
        elem.second.raw_root_sig->Release();
    mCache.clear();
}
