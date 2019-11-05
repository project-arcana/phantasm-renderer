#pragma once

#include <cstdint>

#include <phantasm-renderer/backend/d3d12/common/d3dx12.hh>
#include <phantasm-renderer/backend/d3d12/common/shared_com_ptr.hh>
#include <phantasm-renderer/backend/d3d12/memory/ResourceViewHeaps.hh>
#include <phantasm-renderer/backend/d3d12/memory/UploadHeap.hh>

#include "image_load_util.hh"

namespace pr::backend::d3d12
{
// This class provides functionality to create a 2D-texture from a DDS or any texture format from WIC file.
class Texture
{
public:
    Texture() = default;

    // load file into heap
    bool initFromFile(ID3D12Device& device, UploadHeap& upload_heap, const char* szFilename, bool useSRGB = false, float cutOff = 1.0f);
    INT32 initRenderTarget(ID3D12Device& device, const char* debug_name, CD3DX12_RESOURCE_DESC const& desc, D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_RENDER_TARGET);
    INT32 initDepthStencil(ID3D12Device& device, const char* debug_name, CD3DX12_RESOURCE_DESC const& desc);
    bool initBuffer(ID3D12Device& device, const char* debug_name, CD3DX12_RESOURCE_DESC const& desc, uint32_t structureSize, D3D12_RESOURCE_STATES state); // structureSize needs to be 0 if using a valid DXGI_FORMAT
    bool initCounter(ID3D12Device& device, const char* debug_name, CD3DX12_RESOURCE_DESC const& desc, uint32_t counterSize, D3D12_RESOURCE_STATES state);
    bool initFromData(ID3D12Device& device, const char* debug_name, UploadHeap& upload_heap, const img::image_info& header, const void* data);

    [[nodiscard]] ID3D12Resource* getResource() const { return mResource; }

    void createRTV(uint32_t index, RViewRTV& pRV, int mipLevel = -1);
    void createSRV(uint32_t index, RViewCBV_SRV_UAV& pRV, int mipLevel = -1);
    void createDSV(uint32_t index, RViewDSV& pRV, int arraySlice = 1);
    void createUAV(uint32_t index, RViewCBV_SRV_UAV& pRV);
    void createBufferUAV(uint32_t index, Texture* pCounterTex, RViewCBV_SRV_UAV& pRV); // if counter=true, then UAV is in index, counter is in index+1
    void createCubeSRV(uint32_t index, RViewCBV_SRV_UAV& pRV);

    [[nodiscard]] uint32_t getWidth() const { return mHeader.width; }
    [[nodiscard]] uint32_t getHeight() const { return mHeader.height; }
    [[nodiscard]] uint32_t getMipCount() const { return mHeader.mipMapCount; }

private:
    void createTextureCommitted(ID3D12Device& device, const char* debug_name, bool useSRGB = false);
    void loadAndUpload(ID3D12Device& device, UploadHeap& upload_heap, img::image_handle const& handle);

    void patchFmt24To32Bit(unsigned char* pDst, unsigned char* pSrc, UINT32 pixelCount);
    bool isCubemap() const;

private:
    shared_com_ptr<ID3D12Resource> mResource;

    img::image_info mHeader = {};
    uint32_t mStructuredBufferStride = 0;
};
}
