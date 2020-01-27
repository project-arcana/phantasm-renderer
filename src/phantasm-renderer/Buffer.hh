#pragma once

#include <phantasm-renderer/common/resource_info.hh>
#include <phantasm-renderer/fwd.hh>

namespace pr
{
// ugly workaround
void on_buffer_free(Context* ctx, phi::handle::resource res, uint64_t guid, buffer_info const& info);

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
    Buffer(Context* ctx, phi::handle::resource res, uint64_t guid, buffer_info const& info, std::byte* map = nullptr)
      : mCtx(ctx), mGuid(guid), mResource(res), mInfo(info), mMap(map)
    {
    }

    ~Buffer() { on_buffer_free(mCtx, mResource, mGuid, mInfo); }

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
    Context* mCtx;
    uint64_t mGuid;
    phi::handle::resource mResource; ///< PHI resource
    std::byte* mMap;
    buffer_info mInfo;
};

}
