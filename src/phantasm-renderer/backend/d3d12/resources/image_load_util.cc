#include "image_load_util.hh"
#ifdef PR_BACKEND_D3D12

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>

#include <clean-core/assert.hh>

#include <phantasm-renderer/backend/d3d12/common/d3d12_sanitized.hh>

namespace
{
constexpr auto null_image_handle = pr::backend::d3d12::img::image_handle{nullptr};

/// Returns the last file ending of a given filename
/// Includes the '.'
/// e.g. returns ".png" for "/path/to/myfile.foo.png"
inline std::string get_file_ending(std::string const& str)
{
    auto minPosA = str.rfind('/');
    if (minPosA == std::string::npos)
        minPosA = 0;

    auto minPosB = str.rfind('\\');
    if (minPosB == std::string::npos)
        minPosB = 0;

    auto minPos = minPosA < minPosB ? minPosB : minPosA;

    auto dotPos = str.rfind('.');
    if (dotPos == std::string::npos)
        dotPos = 0;

    if (dotPos <= minPos)
        return ""; // no '.' in actual file name

    return str.substr(dotPos);
}
/// Converts the string to lower
inline std::string to_lowercase(std::string const& str)
{
    auto data = str;
    std::transform(data.begin(), data.end(), data.begin(), ::tolower);
    return data;
}

// DDS loading adapted from AMDUtils
// AMD AMDUtils code
//
// Copyright(c) 2018 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

struct dds_pixelformat
{
    UINT32 size;
    UINT32 flags;
    UINT32 fourCC;
    UINT32 bitCount;
    UINT32 bitMaskR;
    UINT32 bitMaskG;
    UINT32 bitMaskB;
    UINT32 bitMaskA;
};

struct dds_header
{
    UINT32 dwSize;
    UINT32 dwHeaderFlags;
    UINT32 dwHeight;
    UINT32 dwWidth;
    UINT32 dwPitchOrLinearSize;
    UINT32 dwDepth;
    UINT32 dwMipMapCount;
    UINT32 dwReserved1[11];
    dds_pixelformat ddspf;
    UINT32 dwSurfaceFlags;
    UINT32 dwCubemapFlags;
    UINT32 dwCaps3;
    UINT32 dwCaps4;
    UINT32 dwReserved2;
};

//--------------------------------------------------------------------------------------
// retrieve the GetDxGiFormat from a DDS_PIXELFORMAT
// based on http://msdn.microsoft.com/en-us/library/windows/desktop/bb943991(v=vs.85).aspx
//--------------------------------------------------------------------------------------
static DXGI_FORMAT GetDxGiFormat(dds_pixelformat pixelFmt)
{
    if (pixelFmt.flags & 0x00000004) // DDPF_FOURCC
    {
        // Check for D3DFORMAT enums being set here
        switch (pixelFmt.fourCC)
        {
        case '1TXD':
            return DXGI_FORMAT_BC1_UNORM;
        case '3TXD':
            return DXGI_FORMAT_BC2_UNORM;
        case '5TXD':
            return DXGI_FORMAT_BC3_UNORM;
        case 'U4CB':
            return DXGI_FORMAT_BC4_UNORM;
        case 'A4CB':
            return DXGI_FORMAT_BC4_SNORM;
        case '2ITA':
            return DXGI_FORMAT_BC5_UNORM;
        case 'S5CB':
            return DXGI_FORMAT_BC5_SNORM;
        case 'GBGR':
            return DXGI_FORMAT_R8G8_B8G8_UNORM;
        case 'BGRG':
            return DXGI_FORMAT_G8R8_G8B8_UNORM;
        case 36:
            return DXGI_FORMAT_R16G16B16A16_UNORM;
        case 110:
            return DXGI_FORMAT_R16G16B16A16_SNORM;
        case 111:
            return DXGI_FORMAT_R16_FLOAT;
        case 112:
            return DXGI_FORMAT_R16G16_FLOAT;
        case 113:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case 114:
            return DXGI_FORMAT_R32_FLOAT;
        case 115:
            return DXGI_FORMAT_R32G32_FLOAT;
        case 116:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        default:
            return DXGI_FORMAT_UNKNOWN;
        }
    }
    else
    {
        switch (pixelFmt.bitMaskR)
        {
        case 0xff:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case 0x00ff0000:
            return DXGI_FORMAT_B8G8R8A8_UNORM;
        case 0xffff:
            return DXGI_FORMAT_R16G16_UNORM;
        case 0x3ff:
            return DXGI_FORMAT_R10G10B10A2_UNORM;
        case 0x7c00:
            return DXGI_FORMAT_B5G5R5A1_UNORM;
        case 0xf800:
            return DXGI_FORMAT_B5G6R5_UNORM;
        case 0:
            return DXGI_FORMAT_A8_UNORM;
        default:
            return DXGI_FORMAT_UNKNOWN;
        }
    }
}

inline HANDLE load_dds_image(char const* filename, pr::backend::d3d12::img::image_info& out_info)
{
    enum e_resource_dimension
    {
        RESOURCE_DIMENSION_UNKNOWN = 0,
        RESOURCE_DIMENSION_BUFFER = 1,
        RESOURCE_DIMENSION_TEXTURE1D = 2,
        RESOURCE_DIMENSION_TEXTURE2D = 3,
        RESOURCE_DIMENSION_TEXTURE3D = 4
    };

    struct dds_header_dx10
    {
        DXGI_FORMAT dxgiFormat;
        e_resource_dimension resourceDimension;
        UINT32 miscFlag;
        UINT32 arraySize;
        UINT32 reserved;
    };

    if (GetFileAttributesA(filename) == 0xFFFFFFFF)
        return nullptr;

    HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return nullptr;
    }

    LARGE_INTEGER largeFileSize;
    GetFileSizeEx(hFile, &largeFileSize);
    CC_ASSERT(0 == largeFileSize.HighPart);
    UINT32 fileSize = largeFileSize.LowPart;
    UINT32 rawTextureSize = fileSize;

    // read the header
    char headerData[4 + sizeof(dds_header) + sizeof(dds_header_dx10)];
    DWORD dwBytesRead = 0;
    if (ReadFile(hFile, headerData, 4 + sizeof(dds_header) + sizeof(dds_header_dx10), &dwBytesRead, nullptr))
    {
        char* pByteData = headerData;
        UINT32 dwMagic = *reinterpret_cast<UINT32*>(pByteData);
        pByteData += 4;
        rawTextureSize -= 4;

        dds_header* header = reinterpret_cast<dds_header*>(pByteData);
        pByteData += sizeof(dds_header);
        rawTextureSize -= sizeof(dds_header);

        dds_header_dx10* header10 = nullptr;
        if (dwMagic == '01XD') // "DX10"
        {
            header10 = reinterpret_cast<dds_header_dx10*>(&pByteData[4]);
            pByteData += sizeof(dds_header_dx10);
            rawTextureSize -= sizeof(dds_header_dx10);

            out_info = {header->dwWidth,       header->dwHeight,     header->dwDepth,       header10->arraySize,
                        header->dwMipMapCount, header10->dxgiFormat, header->ddspf.bitCount};
        }
        else if (dwMagic == ' SDD') // "DDS "
        {
            // DXGI
            UINT32 arraySize = (header->dwCubemapFlags == 0xfe00) ? 6 : 1;
            DXGI_FORMAT dxgiFormat = GetDxGiFormat(header->ddspf);
            UINT32 mipMapCount = header->dwMipMapCount ? header->dwMipMapCount : 1;

            out_info = {header->dwWidth, header->dwHeight,      header->dwDepth ? header->dwDepth : 1, arraySize, mipMapCount,
                        dxgiFormat,      header->ddspf.bitCount};
        }
        else
        {
            return nullptr;
        }
    }

    ::SetFilePointer(hFile, LONG(fileSize - rawTextureSize), nullptr, FILE_BEGIN);

    return hFile;
}
}


pr::backend::d3d12::img::image_handle pr::backend::d3d12::img::load_image(const char* filename, pr::backend::d3d12::img::image_info& out_info)
{
    auto const extension = to_lowercase(get_file_ending(filename));
    if (extension == ".dds")
    {
        // DDS file
        auto const res_handle = load_dds_image(filename, out_info);
        if (res_handle == nullptr || res_handle == INVALID_HANDLE_VALUE)
            return null_image_handle;
        else
            return {res_handle};
    }
    //    else if (extension == ".wic")
    //    {
    //        // WIC file
    //    }
    else
    {
        // TODO
        std::cerr << "Unknown image format: " << extension << std::endl;
        return null_image_handle;
    }
}

void pr::backend::d3d12::img::copy_pixels(const pr::backend::d3d12::img::image_handle& handle, void* dest, unsigned stride, unsigned width, unsigned height)
{
    for (auto y = 0u; y < height; ++y)
    {
        ::ReadFile(handle.handle, static_cast<char*>(dest) + y * stride, width, nullptr, nullptr);
    }
}

#endif
