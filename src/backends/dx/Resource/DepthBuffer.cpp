#include <Resource/DepthBuffer.h>
#include <Resource/DescriptorHeap.h>
namespace toolhub::directx {
GFXFormat DepthBuffer::GetDepthFormat(DepthFormat f) {
    using namespace luisa::compute;
    switch (f) {
        case DepthFormat::D16:
            return GFXFormat_D16_UNorm;
        case DepthFormat::D24S8:
            return GFXFormat_D24_UNorm_S8_UInt;
        case DepthFormat::D32:
            return GFXFormat_D32_Float;
        default:
            return GFXFormat_D32_Float_S8X24_UInt;
    }
}
DepthBuffer::DepthBuffer(
    Device *device,
    uint width,
    uint height,
    luisa::compute::DepthFormat format,
    IGpuAllocator *alloc)
    : TextureBase(
          device, width, height,
          GetDepthFormat(format),
          TextureDimension::Tex2D, 1, 1),
      allocHandle(alloc) {
}
DepthBuffer::~DepthBuffer() {
    if (srvIdx != ~0u) device->globalHeap->ReturnIndex(srvIdx);
}
D3D12_SHADER_RESOURCE_VIEW_DESC DepthBuffer::GetColorSrvDesc(uint mipOffset) const {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    switch (format) {
        case GFXFormat_D16_UNorm:
            srvDesc.Format = DXGI_FORMAT_R16_UNORM;
            break;
        case GFXFormat_D24_UNorm_S8_UInt:
            srvDesc.Format = DXGI_FORMAT_R32_UINT;
            break;
        case GFXFormat_D32_Float:
            srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
            break;
        case GFXFormat_D32_Float_S8X24_UInt:
            srvDesc.Format = DXGI_FORMAT_R32G32_UINT;
            break;
    }
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    return srvDesc;
}
uint DepthBuffer::GetGlobalSRVIndex(uint mipOffset) const {
    std::lock_guard lck(allocMtx);
    if (srvIdx != ~0u) return srvIdx;
    srvIdx = device->globalHeap->AllocateIndex();
    device->globalHeap->CreateSRV(
        GetResource(),
        GetColorSrvDesc(),
        srvIdx);
    return srvIdx;
}
D3D12_DEPTH_STENCIL_VIEW_DESC DepthBuffer::GetDepthDesc() const {
    D3D12_DEPTH_STENCIL_VIEW_DESC dsv;
    dsv.Format = static_cast<DXGI_FORMAT>(format);
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv.Flags = D3D12_DSV_FLAG_NONE;
    dsv.Texture2D.MipSlice = 0;
    return dsv;
}
}// namespace toolhub::directx