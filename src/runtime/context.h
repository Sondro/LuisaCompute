//
// Created by Mike Smith on 2021/2/2.
//

#pragma once

#include <vector>
#include <filesystem>
#include <unordered_map>

#include <core/stl/memory.h>
#include <core/stl/string.h>
#include <core/dynamic_module.h>

namespace luisa::compute {

class Device;
class BinaryIOVisitor;
struct DeviceSettings {
    size_t device_index;
    bool inqueue_buffer_limit;
};
class LC_RUNTIME_API Context {

private:
    struct Impl;
    luisa::shared_ptr<Impl> _impl;
    size_t _index = ~0ull;
    Context(
        luisa::shared_ptr<Impl> const &impl,
        size_t index);

public:
    explicit Context(const std::filesystem::path &program) noexcept;
    Context(Context &&) noexcept = default;
    Context(const Context &) noexcept = default;
    Context &operator=(Context &&) noexcept = default;
    Context &operator=(const Context &) noexcept = default;
    ~Context() noexcept;
    [[nodiscard]] auto index() const noexcept { return _index; }
    [[nodiscard]] const std::filesystem::path &runtime_directory() const noexcept;
    [[nodiscard]] const std::filesystem::path &cache_directory() const noexcept;
    [[nodiscard]] const std::filesystem::path &data_directory() const noexcept;
    [[nodiscard]] Device create_device(luisa::string_view backend_name, DeviceSettings const* settings = nullptr) noexcept;
    [[nodiscard]] luisa::span<const luisa::string> installed_backends() const noexcept;
    [[nodiscard]] luisa::span<const DynamicModule> loaded_modules() const noexcept;
    [[nodiscard]] Device create_default_device() noexcept;
    [[nodiscard]] BinaryIOVisitor *get_fileio_visitor() const noexcept;
    void set_fileio_visitor(BinaryIOVisitor *file_io) noexcept;
};

}// namespace luisa::compute
