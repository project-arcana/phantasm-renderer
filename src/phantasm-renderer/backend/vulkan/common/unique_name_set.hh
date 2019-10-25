/// Helper to track unique layers and extensions
// TODO: Implement this with a future cc hashmap and vector / iteration helper
#pragma once
#ifdef PR_BACKEND_VULKAN

#include <string>
#include <unordered_set>
#include <vector>

#include <clean-core/span.hh>

#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

namespace pr::backend::vk
{
enum class vk_name_type
{
    layer,
    extension
};

template <vk_name_type T>
struct unique_name_set
{
public:
    void add(std::string const& value) { _names.insert(value); }
    void add(cc::span<std::string const> values)
    {
        for (auto const& v : values)
            _names.insert(v);
    }

    void add(cc::span<VkExtensionProperties const> ext_props)
    {
        static_assert(T == vk_name_type::extension, "Incompatible container");

        for (auto const& ext_prop : ext_props)
            _names.insert(ext_prop.extensionName);
    }

    [[nodiscard]] bool contains(std::string const& name) const { return _names.find(name) != _names.end(); }
    [[nodiscard]] std::vector<std::string> to_vector() const { return std::vector<std::string>(_names.begin(), _names.end()); }

private:
    std::unordered_set<std::string> _names;
};
}

#endif
