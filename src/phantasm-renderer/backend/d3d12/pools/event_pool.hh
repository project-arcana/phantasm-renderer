#pragma once

#include <mutex>

#include <phantasm-renderer/backend/detail/linked_pool.hh>
#include <phantasm-renderer/backend/types.hh>

#include <phantasm-renderer/backend/d3d12/common/d3d12_fwd.hh>

namespace pr::backend::d3d12
{
class EventPool
{
public:
    [[nodiscard]] handle::event createEvent();

    void free(handle::event event);
    void free(cc::span<handle::event const> event_span);

public:
    void initialize(ID3D12Device* device, unsigned max_num_events);
    void destroy();

    ID3D12Fence* get(handle::event event) const
    {
        CC_ASSERT(event.is_valid() && "invalid handle::event");
        return mPool.get(static_cast<unsigned>(event.index));
    }

private:
    ID3D12Device* mDevice = nullptr;

    backend::detail::linked_pool<ID3D12Fence*, unsigned> mPool;

    std::mutex mMutex;
};

}