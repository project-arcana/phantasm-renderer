#include "root_sig_cache.hh"

#include <phantasm-renderer/backend/d3d12/common/util.hh>

void pr::backend::d3d12::RootSignatureCache::initialize(unsigned size_estimate) { mCache.reserve(size_estimate); }

void pr::backend::d3d12::RootSignatureCache::destroy() { reset(); }

pr::backend::d3d12::root_signature* pr::backend::d3d12::RootSignatureCache::getOrCreate(ID3D12Device& device, pr::backend::d3d12::RootSignatureCache::key_t key)
{
    if (auto const it = mCache.find(key); it != mCache.end())
    {
        return &it->second;
    }
    else
    {
        auto [new_it, success] = mCache.emplace(key, root_signature{});
        CC_ASSERT(success);
        initialize_root_signature(new_it->second, device, key.arg_shapes, key.arg_samplers);
        util::set_object_name(new_it->second.raw_root_sig, "RootSignatureCache root sig");
        return &new_it->second;
    }
}

void pr::backend::d3d12::RootSignatureCache::reset()
{
    for (auto const& elem : mCache)
        elem.second.raw_root_sig->Release();
    mCache.clear();
}
