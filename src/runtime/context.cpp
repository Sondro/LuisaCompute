//
// Created by Mike Smith on 2021/2/2.
//

#include <core/logging.h>
#include <core/platform.h>
#include <runtime/context.h>
#include <runtime/device.h>
#include <core/binary_io_visitor.h>

#ifdef LUISA_PLATFORM_WINDOWS
#include <windows.h>
#endif

namespace luisa::compute {
// Make context global, so dynamic modules cannot be over-loaded
struct Context::Impl {
    std::filesystem::path runtime_directory;
    std::filesystem::path cache_directory;
    std::filesystem::path data_directory;
    luisa::vector<DynamicModule> loaded_modules;
    luisa::vector<luisa::string> device_identifiers;
    luisa::vector<Device::Creator *> device_creators;
    luisa::vector<Device::Deleter *> device_deleters;
    luisa::vector<luisa::string> installed_backends;
    BinaryIOVisitor *file_io{nullptr};
};

namespace detail {
[[nodiscard]] auto runtime_directory(const std::filesystem::path &p) noexcept {
    auto cp = std::filesystem::canonical(p);
    if (std::filesystem::is_directory(cp)) { return cp; }
    return std::filesystem::canonical(cp.parent_path());
}
}// namespace detail

Context::Context(const std::filesystem::path &program) noexcept
    : _impl{luisa::make_shared<Impl>()} {
    _impl->runtime_directory = detail::runtime_directory(program);
    LUISA_INFO("Created context for program '{}'.", program.filename().string<char>());
    LUISA_INFO("Runtime directory: {}.", _impl->runtime_directory.string<char>());
    _impl->cache_directory = _impl->runtime_directory / ".cache";
    _impl->data_directory = _impl->runtime_directory / ".data";
    LUISA_INFO("Cache directory: {}.", _impl->cache_directory.string<char>());
    LUISA_INFO("Data directory: {}.", _impl->data_directory.string<char>());
    if (!std::filesystem::exists(_impl->cache_directory)) {
        LUISA_INFO("Created cache directory.");
        std::filesystem::create_directories(_impl->cache_directory);
    }
    DynamicModule::add_search_path(_impl->runtime_directory);
    for (auto &&p : std::filesystem::directory_iterator{_impl->runtime_directory}) {
        if (auto &&path = p.path();
            p.is_regular_file() &&
            (path.extension() == ".so" ||
             path.extension() == ".dll" ||
             path.extension() == ".dylib")) {
            using namespace std::string_view_literals;
            constexpr std::array possible_prefixes{
                "lc-backend-"sv,
                // Make Mingw happy
                "liblc-backend-"sv};
            auto filename = path.stem().string<char, std::char_traits<char>, luisa::allocator<char>>();
            for (auto prefix : possible_prefixes) {
                if (filename.starts_with(prefix)) {
                    auto name = filename.substr(prefix.size());
                    for (auto &c : name) { c = static_cast<char>(std::tolower(c)); }
                    LUISA_VERBOSE_WITH_LOCATION("Found backend: {}.", name);
                    _impl->installed_backends.emplace_back(std::move(name));
                    break;
                }
            }
        }
    }
    std::sort(_impl->installed_backends.begin(), _impl->installed_backends.end());
    _impl->installed_backends.erase(
        std::unique(_impl->installed_backends.begin(), _impl->installed_backends.end()),
        _impl->installed_backends.end());
}

const std::filesystem::path &Context::runtime_directory() const noexcept {
    return _impl->runtime_directory;
}

const std::filesystem::path &Context::cache_directory() const noexcept {
    return _impl->cache_directory;
}

const std::filesystem::path &Context::data_directory() const noexcept {
    return _impl->data_directory;
}

Device Context::create_device(std::string_view backend_name_in,  DeviceSettings const* settings) noexcept {
    luisa::string backend_name{backend_name_in};
    for (auto &c : backend_name) { c = static_cast<char>(std::tolower(c)); }
    auto impl = _impl.get();
    if (std::find(impl->installed_backends.cbegin(),
                  impl->installed_backends.cend(),
                  backend_name) == impl->installed_backends.cend()) {
        LUISA_ERROR_WITH_LOCATION("Backend '{}' is not installed.", backend_name);
    }
    Device::Creator *creator;
    Device::Deleter *deleter;
    size_t index;
    if (auto iter = std::find(impl->device_identifiers.cbegin(),
                              impl->device_identifiers.cend(),
                              backend_name);
        iter != impl->device_identifiers.cend()) {
        index = iter - impl->device_identifiers.cbegin();
        creator = impl->device_creators[index];
        deleter = impl->device_deleters[index];
    } else {
#ifdef LUISA_PLATFORM_WINDOWS
        SetDllDirectoryW(impl->runtime_directory.c_str());
#endif
        auto &&m = impl->loaded_modules.emplace_back(
            impl->runtime_directory,
            fmt::format("lc-backend-{}", backend_name));
#ifdef LUISA_PLATFORM_WINDOWS
        SetDllDirectoryW(nullptr);
#endif
        creator = m.function<Device::Creator>("create");
        deleter = m.function<Device::Deleter>("destroy");
        index = impl->device_identifiers.size();
        impl->device_identifiers.emplace_back(backend_name);
        impl->device_creators.emplace_back(creator);
        impl->device_deleters.emplace_back(deleter);
    }
    return Device{Device::Handle{creator(Context(_impl, index), settings), deleter}};
}

Context::Context(
    luisa::shared_ptr<Impl> const &impl,
    size_t index)
    : _impl(impl),
      _index(index) {}

Context::~Context() noexcept {
    if (_impl != nullptr) {
        DynamicModule::remove_search_path(
            _impl->runtime_directory);
    }
}

luisa::span<const luisa::string> Context::installed_backends() const noexcept {
    return _impl->installed_backends;
}
luisa::span<const DynamicModule> Context::loaded_modules() const noexcept {
    return _impl->loaded_modules;
}

Device Context::create_default_device() noexcept {
    LUISA_ASSERT(!installed_backends().empty(), "No backends installed.");
    return create_device(installed_backends().front());
}

BinaryIOVisitor *Context::get_fileio_visitor() const noexcept {
    return _impl->file_io;
}

void Context::set_fileio_visitor(BinaryIOVisitor *file_io) noexcept {
    _impl->file_io = file_io;
}
}// namespace luisa::compute
