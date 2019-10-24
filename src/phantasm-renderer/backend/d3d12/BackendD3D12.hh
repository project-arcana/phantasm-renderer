#pragma once
#ifdef PR_BACKEND_D3D12

#include <phantasm-renderer/backend/BackendInterface.hh>

namespace pr::backend::d3d12
{
class BackendD3D12 final : public BackendInterface
{
};
}

#endif
