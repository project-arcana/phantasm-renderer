#pragma once

#include <phantasm-renderer/common/resource_info.hh>
#include <phantasm-renderer/fwd.hh>

namespace pr
{
template <class T>
class Buffer
{ // getter
public:
    size_t size() const { return mInfo.size_bytes; }
    size_t stride() const { return mInfo.stride_bytes; }

    bool isReadOnly() const { return !mInfo.allow_uav; }
    bool isMapped() const { return mMap != nullptr; }
    std::byte* getMap() const { return mMap; }

    // ctor
public:
    Buffer(phi::handle::resource res, buffer_info const& info, std::byte* map = nullptr) : mResource(res), mInfo(info), mMap(map) {}

    // internal
public:
    phi::handle::resource getHandle() const { return mResource; }
    buffer_info const& getInfo() const { return mInfo; }

    // move-only
private:
    Buffer(Buffer const&) = delete;
    Buffer(Buffer&&) = default;
    Buffer& operator=(Buffer const&) = delete;
    Buffer& operator=(Buffer&&) = default;

private:
    phi::handle::resource mResource; ///< PHI resource
    std::byte* mMap;
    buffer_info mInfo;
};

}
