#pragma once
#ifdef PR_BACKEND_D3D12

#include <d3d12.h>
#include <dxgi1_6.h>

#include "common/shared_com_ptr.hh"

namespace pr::backend::d3d12
{

class Queue
{
    // reference type
public:
    Queue() = default;
    Queue(Queue const&) = delete;
    Queue(Queue&&) noexcept = delete;
    Queue& operator=(Queue const&) = delete;
    Queue& operator=(Queue&&) noexcept = delete;

    void initialize(ID3D12Device& device, D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);

    [[nodiscard]] ID3D12CommandQueue& getQueue() const { return *mQueue.get(); }
    [[nodiscard]] shared_com_ptr<ID3D12CommandQueue> const& getQueueShared() const { return mQueue; }

private:
    shared_com_ptr<ID3D12CommandQueue> mQueue;
};

}

#endif