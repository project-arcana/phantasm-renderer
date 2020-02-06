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
    unsigned getNumElements() const { return mInfo.stride_bytes == 0 ? 1 : mInfo.size_bytes / mInfo.stride_bytes; }

    // ctor
public:
    Buffer(Context* ctx, uint64_t guid, phi::handle::resource res, buffer_info const& info, std::byte* map = nullptr)
      : mCtx(ctx), mGuid(guid), mResource(res), mInfo(info), mMap(map)
    {
    }

    ~Buffer() { onDestroy(); }

    // internal
public:
    phi::handle::resource getHandle() const { return mResource; }
    buffer_info const& getInfo() const { return mInfo; }

    // move-only
public:
    Buffer(Buffer const&) = delete;
    Buffer& operator=(Buffer const&) = delete;

    Buffer(Buffer&& rhs) noexcept
    {
        mCtx = rhs.mCtx;
        mGuid = rhs.mGuid;
        mResource = rhs.mResource;
        mMap = rhs.mMap;
        mInfo = rhs.mInfo;

        rhs.mCtx = nullptr;
    }
    Buffer& operator=(Buffer&& rhs) noexcept
    {
        if (this != &rhs)
        {
            onDestroy();

            mCtx = rhs.mCtx;
            mGuid = rhs.mGuid;
            mResource = rhs.mResource;
            mMap = rhs.mMap;
            mInfo = rhs.mInfo;

            rhs.mCtx = nullptr;
        }

        return *this;
    }

private:
    void onDestroy()
    {
        if (!mCtx)
            return;

//        detail::on_buffer_free(mCtx, mResource, mGuid, mInfo);
    }

private:
    Context* mCtx = nullptr;
    uint64_t mGuid;
    phi::handle::resource mResource; ///< PHI resource
    std::byte* mMap;
    buffer_info mInfo;
};

}
