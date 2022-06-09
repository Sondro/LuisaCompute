//
// Created by Mike Smith on 2021/7/4.
//

#pragma once

#include <core/basic_types.h>
#include <ast/function_builder.h>
#include <runtime/resource.h>
#include <runtime/bindless_array.h>

namespace luisa::compute {

class Accel;
class BindlessArray;

namespace detail {

template<typename T>
struct prototype_to_shader_invocation {
    using type = const T &;
};

template<typename T>
struct prototype_to_shader_invocation<Buffer<T>> {
    using type = BufferView<T>;
};

template<typename T>
struct prototype_to_shader_invocation<Image<T>> {
    using type = ImageView<T>;
};

template<typename T>
struct prototype_to_shader_invocation<Volume<T>> {
    using type = VolumeView<T>;
};

template<typename T>
using prototype_to_shader_invocation_t = typename prototype_to_shader_invocation<T>::type;

class ShaderInvokeBase {

private:
    ShaderDispatchCommand *_command;
    Function _kernel;
    size_t _argument_index{0u};

private:
public:
    explicit ShaderInvokeBase(uint64_t handle, Function kernel) noexcept
        : _command{ShaderDispatchCommand::create(handle, kernel)},
          _kernel{kernel} {}
    ShaderInvokeBase(ShaderInvokeBase &&) noexcept = default;
    ShaderInvokeBase(const ShaderInvokeBase &) noexcept = delete;
    ShaderInvokeBase &operator=(ShaderInvokeBase &&) noexcept = default;
    ShaderInvokeBase &operator=(const ShaderInvokeBase &) noexcept = delete;

    template<typename T>
    ShaderInvokeBase &operator<<(BufferView<T> buffer) noexcept {
        _command->encode_buffer(buffer.handle(), buffer.offset_bytes(), buffer.size_bytes());
        return *this;
    }

    template<typename T>
    ShaderInvokeBase &operator<<(ImageView<T> image) noexcept {
        _command->encode_texture(image.handle(), image.level());
        return *this;
    }

    template<typename T>
    ShaderInvokeBase &operator<<(VolumeView<T> volume) noexcept {
        _command->encode_texture(volume.handle(), volume.level());
        return *this;
    }

    template<typename T>
    ShaderInvokeBase &operator<<(const Buffer<T> &buffer) noexcept {
        return *this << buffer.view();
    }

    template<typename T>
    ShaderInvokeBase &operator<<(const Image<T> &image) noexcept {
        return *this << image.view();
    }

    template<typename T>
    ShaderInvokeBase &operator<<(const Volume<T> &volume) noexcept {
        return *this << volume.view();
    }

    template<typename T>
    ShaderInvokeBase &operator<<(T data) noexcept {
        _command->encode_uniform(&data, sizeof(T));
        return *this;
    }

    // see definition in rtx/accel.cpp
    ShaderInvokeBase &operator<<(const Accel &accel) noexcept;

    // see definition in runtime/bindless_array.cpp
    ShaderInvokeBase &operator<<(const BindlessArray &array) noexcept;

protected:
    [[nodiscard]] auto _parallelize(uint3 dispatch_size) &&noexcept {
        _command->set_dispatch_size(dispatch_size);
        ShaderDispatchCommand *command{nullptr};
        std::swap(command, _command);
        return command;
    }
};

template<size_t dim>
struct ShaderInvoke {
    static_assert(always_false_v<std::index_sequence<dim>>);
};

template<>
struct ShaderInvoke<1> : public ShaderInvokeBase {
    explicit ShaderInvoke(uint64_t handle, Function kernel) noexcept : ShaderInvokeBase{handle, kernel} {}
    [[nodiscard]] auto dispatch(uint size_x) &&noexcept {
        return std::move(*this)._parallelize(uint3{size_x, 1u, 1u});
    }
};

template<>
struct ShaderInvoke<2> : public ShaderInvokeBase {
    explicit ShaderInvoke(uint64_t handle, Function kernel) noexcept : ShaderInvokeBase{handle, kernel} {}
    [[nodiscard]] auto dispatch(uint size_x, uint size_y) &&noexcept {
        return std::move(*this)._parallelize(uint3{size_x, size_y, 1u});
    }
    [[nodiscard]] auto dispatch(uint2 size) &&noexcept {
        return std::move(*this).dispatch(size.x, size.y);
    }
};

template<>
struct ShaderInvoke<3> : public ShaderInvokeBase {
    explicit ShaderInvoke(uint64_t handle, Function kernel) noexcept : ShaderInvokeBase{handle, kernel} {}
    [[nodiscard]] auto dispatch(uint size_x, uint size_y, uint size_z) &&noexcept {
        return std::move(*this)._parallelize(uint3{size_x, size_y, size_z});
    }
    [[nodiscard]] auto dispatch(uint3 size) &&noexcept {
        return std::move(*this).dispatch(size.x, size.y, size.z);
    }
};

}// namespace detail
template<size_t dimension>
class ShaderBase : public Resource {
protected:
    static_assert(dimension == 1u || dimension == 2u || dimension == 3u);
    luisa::shared_ptr<const detail::FunctionBuilder> _kernel;
    ShaderBase(Device::Interface *device,
               luisa::shared_ptr<const detail::FunctionBuilder> kernel,
               std::string_view meta_options) noexcept
        : Resource{device, Tag::SHADER, device->create_shader(kernel->function(), meta_options)},
          _kernel{std::move(kernel)} {}
    ShaderBase(ShaderBase &&) = default;
    ShaderBase(ShaderBase const &) = delete;
    virtual ~ShaderBase() = default;
};

template<size_t dimension>
class TypelessShader final : public ShaderBase<dimension> {
public:
    using DispatchFunc = detail::ShaderInvoke<dimension> (*)(uint64_t, luisa::shared_ptr<const detail::FunctionBuilder> const &, void **);//   luisa::move_only_function<detail::ShaderInvoke<dimension>(void **)>;

private:
#ifdef _DEBUG
    luisa::vector<std::type_info const *> arg_types;
#endif
    DispatchFunc dispatch_func;

public:
#ifdef _DEBUG
    template<typename Func>
    TypelessShader(ShaderBase<dimension> &&v, luisa::span<std::type_info const *> types, Func &&func)
        : ShaderBase<dimension>(std::move(v)), dispatch_func(std::forward<Func>(func)), arg_types(types.size()) {
        memcpy(arg_types.data(), types.data(), types.size_bytes());
    }
#else
    template<typename Func>
    TypelessShader(ShaderBase<dimension> &&v, Func &&func)
        : ShaderBase<dimension>(std::move(v)), dispatch_func(std::forward<Func>(func)) {
    }
#endif

    template<typename... Args>
    auto operator()(Args &&...args) const {
        void *ptrs[sizeof...(args)];
        size_t index = 0;
#ifdef _DEBUG
        assert(sizeof...(Args) == arg_types.size());
        bool type_match = true;
#endif
        auto check_func = [&](auto &&value) {
#ifdef _DEBUG
            if (typeid(detail::prototype_to_shader_invocation_t<std::remove_cvref_t<decltype(value)>>) != *arg_types[index]) {
                type_match = false;
            }
#endif

            ptrs[index] = &value;
            index++;
        };
        auto call_check_func = {(check_func(args), 0)...};
#ifdef _DEBUG
        assert(type_match);
#endif
        return dispatch_func(handle(), _kernel, ptrs);
    }
};
template<size_t dimension, typename... Args>
class Shader final : public ShaderBase<dimension> {
    friend class Device;
    Shader(Device::Interface *device,
           luisa::shared_ptr<const detail::FunctionBuilder> kernel,
           std::string_view meta_options) noexcept
        : ShaderBase<dimension>(device, std::move(kernel), meta_options) {}

public:
    Shader() noexcept = default;
    using Resource::operator bool;
    [[nodiscard]] auto operator()(detail::prototype_to_shader_invocation_t<Args>... args) const noexcept {
        using invoke_type = detail::ShaderInvoke<dimension>;
        invoke_type invoke{handle(), _kernel->function()};
        return static_cast<invoke_type &&>((invoke << ... << args));
    }
    [[nodiscard]] TypelessShader<dimension> to_typeless() &&noexcept {
        using invoke_type = detail::ShaderInvoke<dimension>;
        auto func = [](uint64_t handle, luisa::shared_ptr<const detail::FunctionBuilder> const &kernel, void **ptrs) -> invoke_type {
            invoke_type invoke{handle, kernel->function()};
            if constexpr (sizeof...(Args) > 0) {
                size_t index = 0;
                auto set_invoke = [&]<typename T> {
                    invoke << *reinterpret_cast<std::remove_cvref_t<T> *>(ptrs[index]);
                    index++;
                };
                auto call_set_invoke = {(set_invoke.template operator()<Args>(), 0)...};
            }
            return std::move(invoke);
        };
#ifdef _DEBUG
        std::type_info const *types[sizeof...(Args)];
        size_t index = 0;
        auto set_types = [&](std::type_info const &t) {
            types[index] = &t;
            index++;
        };
        auto call_set_types = {(set_types(typeid(std::remove_cvref_t<detail::prototype_to_shader_invocation_t<Args>>)), 0)...};
        return TypelessShader<dimension>(
            std::move(*this),
            types,
            func);
#else
        return TypelessShader<dimension>(
            std::move(*this),
            func);
#endif
    }
};


template<typename... Args>
using Shader1D = Shader<1, Args...>;

template<typename... Args>
using Shader2D = Shader<2, Args...>;

template<typename... Args>
using Shader3D = Shader<3, Args...>;

}// namespace luisa::compute
