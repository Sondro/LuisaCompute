#include <Api/LCUtil.h>
#include <Api/LCDevice.h>
#include <DXRuntime/Device.h>
#include <Resource/RenderTexture.h>
#include <Api/LCCmdBuffer.h>
#include <runtime/stream.h>
namespace toolhub::directx {
IUtil *LCDevice::get_util() noexcept {
    if (!util) {
        util = vstd::create_unique(new LCUtil(&nativeDevice));
    }
    return util.get();
}
LCUtil::LCUtil(Device *device)
    : device(device) {
}
LCUtil::~LCUtil() {
}
IUtil::Result LCUtil::compress_bc6h(Stream &stream, Image<float> const &src, luisa::vector<std::byte> &result) noexcept {
    LCCmdBuffer *cmdBuffer = reinterpret_cast<LCCmdBuffer *>(stream.handle());

    RenderTexture *srcTex = reinterpret_cast<RenderTexture *>(src.handle());
    cmdBuffer->CompressBC(
        srcTex,
        result,
        true,
        0,
        device->defaultAllocator.get(),
        LCDevice::maxAllocatorCount);
    return IUtil::Result::Success;
}

IUtil::Result LCUtil::compress_bc7(Stream &stream, Image<float> const &src, luisa::vector<std::byte> &result, float alphaImportance) noexcept {
    LCCmdBuffer *cmdBuffer = reinterpret_cast<LCCmdBuffer *>(stream.handle());
    RenderTexture *srcTex = reinterpret_cast<RenderTexture *>(src.handle());
    cmdBuffer->CompressBC(
        reinterpret_cast<RenderTexture *>(src.handle()),
        result,
        false,
        alphaImportance,
        device->defaultAllocator.get(),
        LCDevice::maxAllocatorCount);
    return IUtil::Result::Success;
}

}// namespace toolhub::directx
