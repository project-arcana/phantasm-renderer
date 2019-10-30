#include "Queue.hh"
#ifdef PR_BACKEND_D3D12

#include "common/verify.hh"

void pr::backend::d3d12::Queue::initialize(ID3D12Device& device, D3D12_COMMAND_LIST_TYPE type)
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = type;

    PR_D3D12_VERIFY(device.CreateCommandQueue(&queueDesc, PR_COM_WRITE(mQueue)));
}


#endif
