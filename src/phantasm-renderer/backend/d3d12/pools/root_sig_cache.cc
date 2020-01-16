#include "root_sig_cache.hh"

#include <phantasm-renderer/backend/d3d12/common/util.hh>

void pr::backend::d3d12::RootSignatureCache::initialize(unsigned max_num_root_sigs) { mCache.initialize(max_num_root_sigs); }

void pr::backend::d3d12::RootSignatureCache::destroy() { reset(); }

pr::backend::d3d12::root_signature* pr::backend::d3d12::RootSignatureCache::getOrCreate(ID3D12Device& device,
                                                                                        arg::shader_argument_shapes arg_shapes,
                                                                                        bool has_root_constants,
                                                                                        bool is_non_graphics)
{
    auto const hash = hashKey(arg_shapes, has_root_constants, is_non_graphics);

    auto* const lookup = mCache.look_up(hash);
    if (lookup != nullptr)
        return lookup;
    else
    {
        auto* const insertion = mCache.insert(hash, root_signature{});
        initialize_root_signature(*insertion, device, arg_shapes, has_root_constants, is_non_graphics);
        util::set_object_name(insertion->raw_root_sig, "cached %sroot sig %zx", is_non_graphics ? "non-graphics " : "graphics ", hash);

        return insertion;
    }
}

void pr::backend::d3d12::RootSignatureCache::reset()
{
    mCache.iterate_elements([](root_signature& root_sig) { root_sig.raw_root_sig->Release(); });
    mCache.clear();
}
