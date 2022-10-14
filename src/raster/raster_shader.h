#pragma once
#include <raster/raster_kernel.h>
#include <dsl/func.h>
#include <runtime/shader.h>
#include <raster/depth_buffer.h>
#include <rtx/accel.h>
#include <runtime/bindless_array.h>
namespace luisa::compute {
class RasterScene;
namespace detail {
template<typename T>
struct PixelDst : public std::false_type {};
template<typename T>
struct PixelDst<Image<T>> : public std::true_type {
    static ShaderDispatchCommandBase::TextureArgument get(Image<T> const &v) {
        return {v.handle(), 0};
    }
};
template<typename T>
struct PixelDst<ImageView<T>> : public std::true_type {
    static ShaderDispatchCommandBase::TextureArgument get(ImageView<T> const &v) {
        return {v.handle(), v.level()};
    }
};
template<typename T, typename... Args>
static constexpr bool LegalDst() {
    constexpr bool r = PixelDst<T>::value;
    if constexpr (sizeof...(Args) == 0) {
        return r;
    } else if constexpr (!r) {
        return false;
    } else {
        return LegalDst<Args...>();
    }
}
}// namespace detail
class RasterShaderInvoke {
private:
    luisa::unique_ptr<DrawRasterSceneCommand> _command;
    Function _vert;
    Function _pixel;

public:
    explicit RasterShaderInvoke(
        uint64_t handle,
        Function vertex_func,
        Function pixel_func)
        : _command(DrawRasterSceneCommand::create(handle, vertex_func, pixel_func)),
          _vert(vertex_func),
          _pixel(pixel_func) {
    }
    RasterShaderInvoke(RasterShaderInvoke &&) noexcept = default;
    RasterShaderInvoke(const RasterShaderInvoke &) noexcept = delete;
    RasterShaderInvoke &operator=(RasterShaderInvoke &&) noexcept = default;
    RasterShaderInvoke &operator=(const RasterShaderInvoke &) noexcept = delete;

    template<typename T>
    RasterShaderInvoke &operator<<(BufferView<T> buffer) noexcept {
        _command->encode_buffer(buffer.handle(), buffer.offset_bytes(), buffer.size_bytes());
        return *this;
    }

    template<typename T>
    RasterShaderInvoke &operator<<(ImageView<T> image) noexcept {
        _command->encode_texture(image.handle(), image.level());
        return *this;
    }

    template<typename T>
    RasterShaderInvoke &operator<<(VolumeView<T> volume) noexcept {
        _command->encode_texture(volume.handle(), volume.level());
        return *this;
    }

    template<typename T>
    RasterShaderInvoke &operator<<(const Buffer<T> &buffer) noexcept {
        return *this << buffer.view();
    }

    template<typename T>
    RasterShaderInvoke &operator<<(const Image<T> &image) noexcept {
        return *this << image.view();
    }

    template<typename T>
    RasterShaderInvoke &operator<<(const Volume<T> &volume) noexcept {
        return *this << volume.view();
    }

    template<typename T>
    RasterShaderInvoke &operator<<(T data) noexcept {
        _command->encode_uniform(&data, sizeof(T));
        return *this;
    }

    // see definition in rtx/accel.cpp
    RasterShaderInvoke &operator<<(const Accel &accel) noexcept {
        _command->encode_accel(accel.handle());
        return *this;
    }

    // see definition in runtime/bindless_array.cpp
    RasterShaderInvoke &operator<<(const BindlessArray &array) noexcept {
        _command->encode_bindless_array(array.handle());
        return *this;
    }
    template<typename... Rtv>
        requires(sizeof...(Rtv) == 0 || detail::LegalDst<Rtv...>())
    [[nodiscard]] auto draw(RasterScene *scene, DepthBuffer const &dsv, Rtv const &...rtv) &&noexcept {
        auto tex_args = {ShaderDispatchCommandBase::TextureArgument(dsv.handle(), 0), detail::PixelDst<std::remove_cvref_t<Rtv>>::get(rtv)...};
        _command->set_dsv_tex(tex_args.begin()[0]);
        _command->set_rtv_texs({tex_args.begin() + 1, tex_args.size() - 1});
        _command->set_scene(scene);
        return std::move(_command);
    }
};

template<typename... Args>
class RasterShader : public Resource {
    friend class Device;
    luisa::shared_ptr<const detail::FunctionBuilder> _vert;
    luisa::shared_ptr<const detail::FunctionBuilder> _pixel;
    // JIT Shader
    RasterShader(
        Device::Interface *device,
        const MeshFormat &mesh_format,
        const RasterState &raster_state,
        luisa::span<PixelFormat const> rtv_format,
        DepthFormat dsv_format,
        luisa::shared_ptr<const detail::FunctionBuilder> vert,
        luisa::shared_ptr<const detail::FunctionBuilder> pixel,
        const std::filesystem::path &file_path)
        : Resource(
              device,
              Tag::RASTER_SHADER,
              device->create_raster_shader(
                  mesh_format,
                  raster_state,
                  rtv_format,
                  dsv_format,
                  Function(vert.get()),
                  Function(pixel.get()),
                  file_path.string<char, std::char_traits<char>, luisa::allocator<char>>())),
          _vert(std::move(vert)),
          _pixel(std::move(pixel)) {
    }
    RasterShader(
        Device::Interface *device,
        const MeshFormat &mesh_format,
        const RasterState &raster_state,
        luisa::span<PixelFormat const> rtv_format,
        DepthFormat dsv_format,
        luisa::shared_ptr<const detail::FunctionBuilder> vert,
        luisa::shared_ptr<const detail::FunctionBuilder> pixel,
        bool use_cache)
        : Resource(
              device,
              Tag::RASTER_SHADER,
              device->create_raster_shader(
                  mesh_format,
                  raster_state,
                  rtv_format,
                  dsv_format,
                  Function(vert.get()),
                  Function(pixel.get()),
                  use_cache)),
          _vert(std::move(vert)),
          _pixel(std::move(pixel)) {
    }
    // AOT Shader
    RasterShader(
        Device::Interface *device,
        const std::filesystem::path &file_path)
        : Resource(
              device,
              Tag::RASTER_SHADER,
              device->load_raster_shader(
                  file_path.string<char, std::char_traits<char>, luisa::allocator<char>>())) {
    }
    [[nodiscard]] auto operator()(detail::prototype_to_shader_invocation_t<Args>... args) const noexcept {
        RasterShaderInvoke invoke(handle(), Function(_vert.get()), Function(_pixel.get()));
        return std::move((invoke << ... << args));
    }
};
}// namespace luisa::compute