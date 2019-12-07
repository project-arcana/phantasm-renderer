#include "root_sig_cache.hh"

#include <phantasm-renderer/backend/d3d12/common/util.hh>

void pr::backend::d3d12::RootSignatureCache::initialize(unsigned max_num_root_sigs) { mCache.initialize(max_num_root_sigs); }

void pr::backend::d3d12::RootSignatureCache::destroy() { reset(); }

pr::backend::d3d12::root_signature* pr::backend::d3d12::RootSignatureCache::getOrCreate(ID3D12Device& device, pr::backend::d3d12::RootSignatureCache::key_t key)
{
    auto const hash = hashKey(key);

    auto* const lookup = mCache.look_up(hash);
    if (lookup != nullptr)
        return lookup;
    else
    {
        auto* const insertion = mCache.insert(hash, root_signature{});
        initialize_root_signature(*insertion, device, key.arg_shapes);
        util::set_object_name(insertion->raw_root_sig, "RootSignatureCache root sig");
        return insertion;
    }
}

void pr::backend::d3d12::RootSignatureCache::reset()
{
    mCache.iterate_elements([](root_signature& root_sig) { root_sig.raw_root_sig->Release(); });
    mCache.clear();
}
