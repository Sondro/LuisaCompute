#include <Api/LCDevice.h>
#include <DXRuntime/Device.h>
#include <Resource/DefaultBuffer.h>
#include <Shader/ShaderCompiler.h>
#include <Resource/RenderTexture.h>
#include <Resource/DepthBuffer.h>
#include <Resource/BindlessArray.h>
#include <Shader/ComputeShader.h>
#include <Shader/RasterShader.h>
#include <Api/LCCmdBuffer.h>
#include <Api/LCEvent.h>
#include <vstl/MD5.h>
#include <Shader/ShaderSerializer.h>
#include <Resource/BottomAccel.h>
#include <Resource/TopAccel.h>
#include <vstl/BinaryReader.h>
#include <Api/LCSwapChain.h>
#include <Api/LCUtil.h>
#include "HLSL/dx_codegen.h"
#include <remote/DatabaseInclude.h>
#include <ast/function_builder.h>
#include <Resource/DepthBuffer.h>
using namespace toolhub::directx;
namespace toolhub::directx {
static constexpr uint kShaderModel = 65u;
LCDevice::LCDevice(Context &&ctx, DeviceSettings const *settings)
    : LCDeviceInterface(std::move(ctx)),
      shaderPaths{},
      nativeDevice(shaderPaths, settings ? settings->device_index : size_t(0)) {
    if (settings) {
        nativeDevice.maxAllocatorCount = settings->inqueue_buffer_limit ? 2 : std::numeric_limits<size_t>::max();
    }
    shaderPaths.shaderCacheFolder = _ctx.cache_directory().string<char, std::char_traits<char>, luisa::allocator<char>>();
    shaderPaths.dataFolder = (_ctx.data_directory() / "dx_builtin").string<char, std::char_traits<char>, luisa::allocator<char>>();
    auto ProcessPath = [](vstd::string &str) {
        for (auto &&i : str) {
            if (i == '\\') i = '/';
        }
        if (*(str.end() - 1) != '/') str << '/';
    };
    ProcessPath(shaderPaths.shaderCacheFolder);
    ProcessPath(shaderPaths.dataFolder);
}
LCDevice::~LCDevice() {
}
void *LCDevice::native_handle() const noexcept {
    return nativeDevice.device.Get();
}
uint64 LCDevice::create_buffer(size_t size_bytes) noexcept {
    return reinterpret_cast<uint64>(
        static_cast<Buffer *>(
            new DefaultBuffer(
                &nativeDevice,
                size_bytes,
                nativeDevice.defaultAllocator.get())));
}
void LCDevice::destroy_buffer(uint64 handle) noexcept {
    delete reinterpret_cast<Buffer *>(handle);
}
void *LCDevice::buffer_native_handle(uint64 handle) const noexcept {
    return reinterpret_cast<Buffer *>(handle)->GetResource();
}
uint64 LCDevice::create_texture(
    PixelFormat format,
    uint dimension,
    uint width,
    uint height,
    uint depth,
    uint mipmap_levels) noexcept {
    bool allowUAV = true;
    switch (format) {
        case PixelFormat::BC4UNorm:
        case PixelFormat::BC5UNorm:
        case PixelFormat::BC6HUF16:
        case PixelFormat::BC7UNorm:
            allowUAV = false;
            break;
    }
    return reinterpret_cast<uint64>(
        static_cast<TextureBase *>(
            new RenderTexture(
                &nativeDevice,
                width,
                height,
                TextureBase::ToGFXFormat(format),
                (TextureDimension)dimension,
                depth,
                mipmap_levels,
                allowUAV,
                nativeDevice.defaultAllocator.get())));
}
void LCDevice::destroy_texture(uint64 handle) noexcept {
    delete reinterpret_cast<TextureBase *>(handle);
}
void *LCDevice::texture_native_handle(uint64 handle) const noexcept {
    return reinterpret_cast<TextureBase *>(handle)->GetResource();
}
uint64 LCDevice::create_bindless_array(size_t size) noexcept {
    return reinterpret_cast<uint64>(
        new BindlessArray(&nativeDevice, size));
}
void LCDevice::destroy_bindless_array(uint64 handle) noexcept {
    delete reinterpret_cast<BindlessArray *>(handle);
}
void LCDevice::emplace_buffer_in_bindless_array(uint64 array, size_t index, uint64 handle, size_t offset_bytes) noexcept {
    auto buffer = reinterpret_cast<Buffer *>(handle);
    reinterpret_cast<BindlessArray *>(array)
        ->Bind(handle, BufferView(buffer, offset_bytes), index);
}
void LCDevice::emplace_tex2d_in_bindless_array(uint64 array, size_t index, uint64 handle, Sampler sampler) noexcept {
    auto tex = reinterpret_cast<TextureBase *>(handle);
    reinterpret_cast<BindlessArray *>(array)
        ->Bind(handle, std::pair<TextureBase const *, Sampler>(tex, sampler), index);
}
void LCDevice::emplace_tex3d_in_bindless_array(uint64 array, size_t index, uint64 handle, Sampler sampler) noexcept {
    emplace_tex2d_in_bindless_array(array, index, handle, sampler);
}
/*
bool LCDevice::is_resource_in_bindless_array(uint64 array, uint64 handle) const noexcept {

}*/
void LCDevice::remove_buffer_in_bindless_array(uint64 array, size_t index) noexcept {
    reinterpret_cast<BindlessArray *>(array)
        ->UnBind(BindlessArray::BindTag::Buffer, index);
}
void LCDevice::remove_tex2d_in_bindless_array(uint64 array, size_t index) noexcept {
    reinterpret_cast<BindlessArray *>(array)
        ->UnBind(BindlessArray::BindTag::Tex2D, index);
}
void LCDevice::remove_tex3d_in_bindless_array(uint64 array, size_t index) noexcept {
    reinterpret_cast<BindlessArray *>(array)
        ->UnBind(BindlessArray::BindTag::Tex3D, index);
}
uint64 LCDevice::create_stream(StreamTag type) noexcept {
    return reinterpret_cast<uint64>(
        new LCCmdBuffer(
            &nativeDevice,
            nativeDevice.defaultAllocator.get(),
            [&] {
                switch (type) {
                    case luisa::compute::StreamTag::COMPUTE:
                        return D3D12_COMMAND_LIST_TYPE_COMPUTE;
                    case luisa::compute::StreamTag::GRAPHICS:
                        return D3D12_COMMAND_LIST_TYPE_DIRECT;
                    case luisa::compute::StreamTag::COPY:
                        return D3D12_COMMAND_LIST_TYPE_COPY;
                }
            }()));
}

void LCDevice::destroy_stream(uint64 handle) noexcept {
    delete reinterpret_cast<LCCmdBuffer *>(handle);
}
void LCDevice::synchronize_stream(uint64 stream_handle) noexcept {
    reinterpret_cast<LCCmdBuffer *>(stream_handle)->Sync();
}
void LCDevice::dispatch(uint64 stream_handle, CommandList &&list) noexcept {
    reinterpret_cast<LCCmdBuffer *>(stream_handle)
        ->Execute(std::move(list), nativeDevice.maxAllocatorCount);
}
void LCDevice::dispatch(uint64 stream_handle, luisa::move_only_function<void()> &&func) noexcept {
    reinterpret_cast<LCCmdBuffer *>(stream_handle)->queue.Callback(std::move(func));
}

void *LCDevice::stream_native_handle(uint64 handle) const noexcept {
    return reinterpret_cast<LCCmdBuffer *>(handle)
        ->queue.Queue();
}
void LCDevice::set_io_visitor(BinaryIOVisitor *visitor) noexcept {
    if (visitor) {
        nativeDevice.fileIo = visitor;
    } else {
        nativeDevice.fileIo = &nativeDevice.serVisitor;
    }
}

uint64 LCDevice::create_shader(Function kernel, std::string_view file_name) noexcept {
    auto code = CodegenUtility::Codegen(kernel, shaderPaths.dataFolder);
    vstd::MD5 checkMD5({reinterpret_cast<vbyte const *>(code.result.data() + code.immutableHeaderSize), code.result.size() - code.immutableHeaderSize});
    return reinterpret_cast<uint64>(
        ComputeShader::CompileCompute(
            nativeDevice.fileIo,
            &nativeDevice,
            kernel,
            [&]() { return std::move(code); },
            checkMD5,
            kernel.block_size(),
            kShaderModel,
            file_name,
            false));
}
uint64 LCDevice::create_shader(Function kernel, bool use_cache) noexcept {
    auto code = CodegenUtility::Codegen(kernel, shaderPaths.dataFolder);
    vstd::string file_name;
    vstd::MD5 checkMD5({reinterpret_cast<vbyte const *>(code.result.data() + code.immutableHeaderSize), code.result.size() - code.immutableHeaderSize});
    if (use_cache) {
        file_name << checkMD5.ToString() << ".dxil";
    }
    return reinterpret_cast<uint64>(
        ComputeShader::CompileCompute(
            nativeDevice.fileIo,
            &nativeDevice,
            kernel,
            [&]() { return std::move(code); },
            checkMD5,
            kernel.block_size(),
            kShaderModel,
            file_name,
            true));
}
uint64 LCDevice::load_shader(
    vstd::string_view file_name,
    vstd::span<Type const *const> types) noexcept {
    auto cs = ComputeShader::LoadPresetCompute(
        nativeDevice.fileIo,
        &nativeDevice,
        types,
        file_name);
    if (cs)
        return reinterpret_cast<uint64>(cs);
    else
        return ~0;
}
void LCDevice::save_shader(Function kernel, luisa::string_view file_name) noexcept {
    auto code = CodegenUtility::Codegen(kernel, shaderPaths.dataFolder);
    ComputeShader::SaveCompute(
        nativeDevice.fileIo,
        kernel,
        code,
        kernel.block_size(),
        kShaderModel,
        file_name);
}
void LCDevice::destroy_shader(uint64 handle) noexcept {
    auto shader = reinterpret_cast<Shader *>(handle);
    delete shader;
}
uint64 LCDevice::create_event() noexcept {
    return reinterpret_cast<uint64>(
        new LCEvent(&nativeDevice));
}
void LCDevice::destroy_event(uint64 handle) noexcept {
    delete reinterpret_cast<LCEvent *>(handle);
}
void LCDevice::signal_event(uint64 handle, uint64 stream_handle) noexcept {
    reinterpret_cast<LCEvent *>(handle)->Signal(
        &reinterpret_cast<LCCmdBuffer *>(stream_handle)->queue);
}

void LCDevice::wait_event(uint64 handle, uint64 stream_handle) noexcept {
    reinterpret_cast<LCEvent *>(handle)->Wait(
        &reinterpret_cast<LCCmdBuffer *>(stream_handle)->queue);
}
void LCDevice::synchronize_event(uint64 handle) noexcept {
    reinterpret_cast<LCEvent *>(handle)->Sync();
}

uint64 LCDevice::create_mesh(
    AccelUsageHint hint,
    bool allow_compact, bool allow_update) noexcept {
    return reinterpret_cast<uint64>(
        (
            new BottomAccel(
                &nativeDevice,
                hint,
                allow_compact,
                allow_update)));
}
void LCDevice::destroy_mesh(uint64 handle) noexcept {
    delete reinterpret_cast<BottomAccel *>(handle);
}
uint64 LCDevice::create_accel(AccelUsageHint hint, bool allow_compact, bool allow_update) noexcept {
    return reinterpret_cast<uint64>(new TopAccel(
        &nativeDevice,
        hint,
        allow_compact,
        allow_update));
}
void LCDevice::destroy_accel(uint64 handle) noexcept {
    delete reinterpret_cast<TopAccel *>(handle);
}
uint64 LCDevice::create_swap_chain(
    uint64 window_handle,
    uint64 stream_handle,
    uint width,
    uint height,
    bool allow_hdr,
    uint back_buffer_size) noexcept {
    return reinterpret_cast<uint64>(
        new LCSwapChain(
            &nativeDevice,
            &reinterpret_cast<LCCmdBuffer *>(stream_handle)->queue,
            nativeDevice.defaultAllocator.get(),
            reinterpret_cast<HWND>(window_handle),
            width,
            height,
            allow_hdr,
            back_buffer_size));
}
void LCDevice::destroy_swap_chain(uint64 handle) noexcept {
    delete reinterpret_cast<LCSwapChain *>(handle);
}
PixelStorage LCDevice::swap_chain_pixel_storage(uint64 handle) noexcept {
    return PixelStorage::BYTE4;
}
void LCDevice::present_display_in_stream(uint64 stream_handle, uint64 swapchain_handle, uint64 image_handle) noexcept {
    reinterpret_cast<LCCmdBuffer *>(stream_handle)
        ->Present(
            reinterpret_cast<LCSwapChain *>(swapchain_handle),
            reinterpret_cast<TextureBase *>(image_handle));
}

uint64_t LCDevice::create_raster_shader(
    const MeshFormat &mesh_format,
    const RasterState &raster_state,
    luisa::span<const PixelFormat> rtv_format,
    DepthFormat dsv_format,
    Function vert,
    Function pixel,
    luisa::string_view file_name) noexcept {
    auto code = CodegenUtility::RasterCodegen(mesh_format, vert, pixel, shaderPaths.dataFolder);
    vstd::MD5 checkMD5({reinterpret_cast<vbyte const *>(code.result.data() + code.immutableHeaderSize), code.result.size() - code.immutableHeaderSize});
    auto shader = RasterShader::CompileRaster(
        nativeDevice.fileIo,
        &nativeDevice,
        vert,
        pixel,
        [&] { return std::move(code); },
        checkMD5,
        kShaderModel,
        mesh_format,
        raster_state,
        rtv_format,
        dsv_format,
        file_name,
        true);
    return reinterpret_cast<uint64>(shader);
}

uint64_t LCDevice::load_raster_shader(
    const MeshFormat &mesh_format,
    const RasterState &raster_state,
    luisa::span<const PixelFormat> rtv_format,
    DepthFormat dsv_format,
    luisa::string_view ser_path) noexcept {
    auto ptr = RasterShader::LoadRaster(
        nativeDevice.fileIo,
        &nativeDevice,
        mesh_format,
        raster_state,
        rtv_format,
        dsv_format,
        ser_path);
    if (ptr) return reinterpret_cast<uint64>(ptr);
    return ~0ull;
}
void LCDevice::save_raster_shader(
    const MeshFormat &mesh_format,
    Function vert,
    Function pixel,
    luisa::string_view file_name) noexcept {
    auto code = CodegenUtility::RasterCodegen(mesh_format, vert, pixel, shaderPaths.dataFolder);
    vstd::MD5 checkMD5({reinterpret_cast<vbyte const *>(code.result.data() + code.immutableHeaderSize), code.result.size() - code.immutableHeaderSize});
    RasterShader::SaveRaster(
        nativeDevice.fileIo,
        &nativeDevice,
        code,
        checkMD5,
        file_name,
        vert,
        pixel,
        kShaderModel);
}

uint64_t LCDevice::create_raster_shader(
    const MeshFormat &mesh_format,
    const RasterState &raster_state,
    luisa::span<const PixelFormat> rtv_format,
    DepthFormat dsv_format,
    Function vert,
    Function pixel,
    bool use_cache) noexcept {
    auto code = CodegenUtility::RasterCodegen(mesh_format, vert, pixel, shaderPaths.dataFolder);
    vstd::string file_name;
    vstd::MD5 checkMD5({reinterpret_cast<vbyte const *>(code.result.data() + code.immutableHeaderSize), code.result.size() - code.immutableHeaderSize});
    if (use_cache) {
        file_name << checkMD5.ToString() << ".dxil";
    }
    auto shader = RasterShader::CompileRaster(
        nativeDevice.fileIo,
        &nativeDevice,
        vert,
        pixel,
        [&] { return std::move(code); },
        checkMD5,
        kShaderModel,
        mesh_format,
        raster_state,
        rtv_format,
        dsv_format,
        file_name,
        true);
    return reinterpret_cast<uint64>(shader);
}
uint64_t LCDevice::create_depth_buffer(DepthFormat format, uint width, uint height) noexcept {
    return reinterpret_cast<uint64_t>(
        static_cast<TextureBase *>(
            new DepthBuffer(
                &nativeDevice,
                width, height,
                format, nativeDevice.defaultAllocator.get())));
}

void LCDevice::destroy_depth_buffer(uint64_t handle) noexcept {
    delete reinterpret_cast<TextureBase *>(handle);
}

VSTL_EXPORT_C LCDeviceInterface *create(Context &&c, DeviceSettings const *settings) {
    return new LCDevice(std::move(c), settings);
}
VSTL_EXPORT_C void destroy(LCDeviceInterface *device) {
    delete static_cast<LCDevice *>(device);
}
}// namespace toolhub::directx