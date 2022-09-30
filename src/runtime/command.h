//
// Created by Mike Smith on 2021/3/3.
//

#pragma once

#include <cstdint>
#include <cstddef>
#include <variant>
#include <memory>
#include <vector>
#include <array>
#include <span>

#include <core/clock.h>
#include <core/logging.h>
#include <core/basic_types.h>
#include <core/pool.h>
#include <ast/variable.h>
#include <ast/function.h>
#include <runtime/pixel.h>
#include <ast/function_builder.h>

namespace luisa::compute {

#define LUISA_COMPUTE_RUNTIME_COMMANDS \
    BufferUploadCommand,               \
        BufferDownloadCommand,         \
        BufferCopyCommand,             \
        BufferToTextureCopyCommand,    \
        ShaderDispatchCommand,         \
        TextureUploadCommand,          \
        TextureDownloadCommand,        \
        TextureCopyCommand,            \
        TextureToBufferCopyCommand,    \
        AccelBuildCommand,             \
        MeshBuildCommand,              \
        BindlessArrayUpdateCommand

#define LUISA_MAKE_COMMAND_FWD_DECL(CMD) class CMD;
LUISA_MAP(LUISA_MAKE_COMMAND_FWD_DECL, LUISA_COMPUTE_RUNTIME_COMMANDS)
#undef LUISA_MAKE_COMMAND_FWD_DECL

struct CommandVisitor {
#define LUISA_MAKE_COMMAND_VISITOR_INTERFACE(CMD) \
    virtual void visit(const CMD *) noexcept = 0;
    LUISA_MAP(LUISA_MAKE_COMMAND_VISITOR_INTERFACE, LUISA_COMPUTE_RUNTIME_COMMANDS)
#undef LUISA_MAKE_COMMAND_VISITOR_INTERFACE
};

struct MutableCommandVisitor {
#define LUISA_MAKE_COMMAND_VISITOR_INTERFACE(CMD) \
    virtual void visit(CMD *) noexcept = 0;
    LUISA_MAP(LUISA_MAKE_COMMAND_VISITOR_INTERFACE, LUISA_COMPUTE_RUNTIME_COMMANDS)
#undef LUISA_MAKE_COMMAND_VISITOR_INTERFACE
};

class Command;
class CommandList;

namespace detail {

#define LUISA_MAKE_COMMAND_POOL_DECL(Cmd) \
    LC_RUNTIME_API Pool<Cmd> &pool_##Cmd() noexcept;
LUISA_MAP(LUISA_MAKE_COMMAND_POOL_DECL, LUISA_COMPUTE_RUNTIME_COMMANDS)
#undef LUISA_MAKE_COMMAND_POOL_DECL

}// namespace detail

#define LUISA_MAKE_COMMAND_COMMON_CREATE(Cmd)                                    \
    template<typename... Args>                                                   \
    [[nodiscard]] static auto create(Args &&...args) noexcept {                  \
        Clock clock;                                                             \
        auto command = detail::pool_##Cmd().create(std::forward<Args>(args)...); \
        LUISA_VERBOSE_WITH_LOCATION(                                             \
            "Created {} in {} ms.", #Cmd, clock.toc());                          \
        return command;                                                          \
    }

#define LUISA_MAKE_COMMAND_COMMON_ACCEPT(Cmd)                                             \
    void accept(CommandVisitor &visitor) const noexcept override { visitor.visit(this); } \
    void accept(MutableCommandVisitor &visitor) noexcept override { visitor.visit(this); }

#define LUISA_MAKE_COMMAND_COMMON_RECYCLE(Cmd) \
    void _recycle() noexcept override { detail::pool_##Cmd().recycle(this); }

#define LUISA_MAKE_COMMAND_COMMON(Cmd)    \
    LUISA_MAKE_COMMAND_COMMON_CREATE(Cmd) \
    LUISA_MAKE_COMMAND_COMMON_ACCEPT(Cmd) \
    LUISA_MAKE_COMMAND_COMMON_RECYCLE(Cmd)

class LC_RUNTIME_API Command {

public:
    enum struct Tag {
#define LUISA_MAKE_COMMAND_TAG(Cmd) E##Cmd,
        LUISA_MAP(LUISA_MAKE_COMMAND_TAG, LUISA_COMPUTE_RUNTIME_COMMANDS)
#undef LUISA_MAKE_COMMAND_TAG
    };

private:
    Tag _tag;

protected:
    virtual void _recycle() noexcept = 0;

public:
    explicit Command(Tag tag) noexcept : _tag(tag) {}
    virtual ~Command() noexcept = default;
    virtual void accept(CommandVisitor &visitor) const noexcept = 0;
    virtual void accept(MutableCommandVisitor &visitor) noexcept = 0;
    void recycle();
    [[nodiscard]] auto tag() const noexcept { return _tag; }
};

class LC_RUNTIME_API BufferUploadCommand final : public Command {

private:
    uint64_t _handle{};
    size_t _offset{};
    size_t _size{};
    const void *_data{};

private:
    BufferUploadCommand() noexcept
        : Command{Command::Tag::EBufferUploadCommand} {}

public:
    BufferUploadCommand(uint64_t handle, size_t offset_bytes, size_t size_bytes, const void *data) noexcept
        : Command{Command::Tag::EBufferUploadCommand},
          _handle{handle}, _offset{offset_bytes}, _size{size_bytes}, _data{data} {}
    [[nodiscard]] auto handle() const noexcept { return _handle; }
    [[nodiscard]] auto offset() const noexcept { return _offset; }
    [[nodiscard]] auto size() const noexcept { return _size; }
    [[nodiscard]] auto data() const noexcept { return _data; }
    LUISA_MAKE_COMMAND_COMMON(BufferUploadCommand)
};

class LC_RUNTIME_API BufferDownloadCommand final : public Command {

private:
    uint64_t _handle{};
    size_t _offset{};
    size_t _size{};
    void *_data{};

private:
    BufferDownloadCommand() noexcept
        : Command{Command::Tag::EBufferDownloadCommand} {}

public:
    BufferDownloadCommand(uint64_t handle, size_t offset_bytes, size_t size_bytes, void *data) noexcept
        : Command{Command::Tag::EBufferDownloadCommand},
          _handle{handle}, _offset{offset_bytes}, _size{size_bytes}, _data{data} {}
    [[nodiscard]] auto handle() const noexcept { return _handle; }
    [[nodiscard]] auto offset() const noexcept { return _offset; }
    [[nodiscard]] auto size() const noexcept { return _size; }
    [[nodiscard]] auto data() const noexcept { return _data; }
    LUISA_MAKE_COMMAND_COMMON(BufferDownloadCommand)
};

class LC_RUNTIME_API BufferCopyCommand final : public Command {

private:
    uint64_t _src_handle{};
    uint64_t _dst_handle{};
    size_t _src_offset{};
    size_t _dst_offset{};
    size_t _size{};

private:
    BufferCopyCommand() noexcept
        : Command{Command::Tag::EBufferCopyCommand} {}

public:
    BufferCopyCommand(uint64_t src, uint64_t dst, size_t src_offset, size_t dst_offset, size_t size) noexcept
        : Command{Command::Tag::EBufferCopyCommand},
          _src_handle{src}, _dst_handle{dst},
          _src_offset{src_offset}, _dst_offset{dst_offset}, _size{size} {}
    [[nodiscard]] auto src_handle() const noexcept { return _src_handle; }
    [[nodiscard]] auto dst_handle() const noexcept { return _dst_handle; }
    [[nodiscard]] auto src_offset() const noexcept { return _src_offset; }
    [[nodiscard]] auto dst_offset() const noexcept { return _dst_offset; }
    [[nodiscard]] auto size() const noexcept { return _size; }
    LUISA_MAKE_COMMAND_COMMON(BufferCopyCommand)
};

class LC_RUNTIME_API BufferToTextureCopyCommand final : public Command {

private:
    uint64_t _buffer_handle{};
    size_t _buffer_offset{};
    uint64_t _texture_handle{};
    PixelStorage _pixel_storage{};
    uint _texture_level{};
    uint _texture_size[3]{};

private:
    BufferToTextureCopyCommand() noexcept
        : Command{Command::Tag::EBufferToTextureCopyCommand} {}

public:
    BufferToTextureCopyCommand(uint64_t buffer, size_t buffer_offset,
                               uint64_t texture, PixelStorage storage,
                               uint level, uint3 size) noexcept
        : Command{Command::Tag::EBufferToTextureCopyCommand},
          _buffer_handle{buffer}, _buffer_offset{buffer_offset},
          _texture_handle{texture}, _pixel_storage{storage}, _texture_level{level},
          _texture_size{size.x, size.y, size.z} {}
    [[nodiscard]] auto buffer() const noexcept { return _buffer_handle; }
    [[nodiscard]] auto buffer_offset() const noexcept { return _buffer_offset; }
    [[nodiscard]] auto texture() const noexcept { return _texture_handle; }
    [[nodiscard]] auto storage() const noexcept { return _pixel_storage; }
    [[nodiscard]] auto level() const noexcept { return _texture_level; }
    [[nodiscard]] auto size() const noexcept { return uint3(_texture_size[0], _texture_size[1], _texture_size[2]); }
    LUISA_MAKE_COMMAND_COMMON(BufferToTextureCopyCommand)
};

class LC_RUNTIME_API TextureToBufferCopyCommand final : public Command {

private:
    uint64_t _buffer_handle{};
    size_t _buffer_offset{};
    uint64_t _texture_handle{};
    PixelStorage _pixel_storage{};
    uint _texture_level{};
    uint _texture_size[3]{};

private:
    TextureToBufferCopyCommand() noexcept
        : Command{Command::Tag::ETextureToBufferCopyCommand} {}

public:
    TextureToBufferCopyCommand(uint64_t buffer, size_t buffer_offset,
                               uint64_t texture, PixelStorage storage,
                               uint level, uint3 size) noexcept
        : Command{Command::Tag::ETextureToBufferCopyCommand},
          _buffer_handle{buffer}, _buffer_offset{buffer_offset},
          _texture_handle{texture}, _pixel_storage{storage}, _texture_level{level},
          _texture_size{size.x, size.y, size.z} {}
    [[nodiscard]] auto buffer() const noexcept { return _buffer_handle; }
    [[nodiscard]] auto buffer_offset() const noexcept { return _buffer_offset; }
    [[nodiscard]] auto texture() const noexcept { return _texture_handle; }
    [[nodiscard]] auto storage() const noexcept { return _pixel_storage; }
    [[nodiscard]] auto level() const noexcept { return _texture_level; }
    [[nodiscard]] auto size() const noexcept { return uint3(_texture_size[0], _texture_size[1], _texture_size[2]); }
    LUISA_MAKE_COMMAND_COMMON(TextureToBufferCopyCommand)
};

class LC_RUNTIME_API TextureCopyCommand final : public Command {

private:
    PixelStorage _storage{};
    uint64_t _src_handle{};
    uint64_t _dst_handle{};
    uint _size[3]{};
    uint _src_level{};
    uint _dst_level{};

private:
    TextureCopyCommand() noexcept
        : Command{Command::Tag::ETextureCopyCommand} {}

public:
    TextureCopyCommand(PixelStorage storage, uint64_t src_handle, uint64_t dst_handle,
                       uint src_level, uint dst_level, uint3 size) noexcept
        : Command{Command::Tag::ETextureCopyCommand},
          _storage{storage}, _src_handle{src_handle}, _dst_handle{dst_handle},
          _size{size.x, size.y, size.z}, _src_level{src_level}, _dst_level{dst_level} {}
    [[nodiscard]] auto storage() const noexcept { return _storage; }
    [[nodiscard]] auto src_handle() const noexcept { return _src_handle; }
    [[nodiscard]] auto dst_handle() const noexcept { return _dst_handle; }
    [[nodiscard]] auto size() const noexcept { return uint3(_size[0], _size[1], _size[2]); }
    [[nodiscard]] auto src_level() const noexcept { return _src_level; }
    [[nodiscard]] auto dst_level() const noexcept { return _dst_level; }
    LUISA_MAKE_COMMAND_COMMON(TextureCopyCommand)
};

class LC_RUNTIME_API TextureUploadCommand final : public Command {

private:
    uint64_t _handle{};
    PixelStorage _storage{};
    uint _level{};
    uint _size[3]{};
    const void *_data{};

private:
    TextureUploadCommand() noexcept
        : Command{Command::Tag::ETextureUploadCommand} {}

public:
    TextureUploadCommand(uint64_t handle, PixelStorage storage,
                         uint level, uint3 size, const void *data) noexcept
        : Command{Command::Tag::ETextureUploadCommand},
          _handle{handle}, _storage{storage}, _level{level},
          _size{size.x, size.y, size.z}, _data{data} {}
    [[nodiscard]] auto handle() const noexcept { return _handle; }
    [[nodiscard]] auto storage() const noexcept { return _storage; }
    [[nodiscard]] auto level() const noexcept { return _level; }
    [[nodiscard]] auto size() const noexcept { return uint3(_size[0], _size[1], _size[2]); }
    [[nodiscard]] auto data() const noexcept { return _data; }
    LUISA_MAKE_COMMAND_COMMON(TextureUploadCommand)
};

class LC_RUNTIME_API TextureDownloadCommand final : public Command {

private:
    uint64_t _handle{};
    PixelStorage _storage{};
    uint _level{};
    uint _size[3]{};
    void *_data{};

private:
    TextureDownloadCommand() noexcept
        : Command{Command::Tag::ETextureDownloadCommand} {}

public:
    TextureDownloadCommand(uint64_t handle, PixelStorage storage,
                           uint level, uint3 size, void *data) noexcept
        : Command{Command::Tag::ETextureDownloadCommand},
          _handle{handle}, _storage{storage}, _level{level},
          _size{size.x, size.y, size.z}, _data{data} {}
    [[nodiscard]] auto handle() const noexcept { return _handle; }
    [[nodiscard]] auto storage() const noexcept { return _storage; }
    [[nodiscard]] auto level() const noexcept { return _level; }
    [[nodiscard]] auto size() const noexcept { return uint3(_size[0], _size[1], _size[2]); }
    [[nodiscard]] auto data() const noexcept { return _data; }
    LUISA_MAKE_COMMAND_COMMON(TextureDownloadCommand)
};

class LC_RUNTIME_API ShaderDispatchCommand final : public Command {

public:
    struct alignas(8) Argument {

        enum struct Tag : uint32_t {
            BUFFER,
            TEXTURE,
            UNIFORM,
            BINDLESS_ARRAY,
            ACCEL,
        };

        Tag tag{};

        Argument() noexcept = default;
        explicit Argument(Tag tag) noexcept : tag{tag} {}
    };

    struct BufferArgument : Argument {
        uint64_t handle{};
        size_t offset{};
        size_t size{};
        BufferArgument() noexcept : Argument{Tag::BUFFER} {}
        BufferArgument(uint64_t handle, size_t offset, size_t size) noexcept
            : Argument{Tag::BUFFER}, handle{handle}, offset{offset}, size{size} {}
    };

    struct TextureArgument : Argument {
        uint64_t handle{};
        uint32_t level{};
        TextureArgument() noexcept : Argument{Tag::TEXTURE} {}
        TextureArgument(uint64_t handle, uint32_t level) noexcept
            : Argument{Tag::TEXTURE}, handle{handle}, level{level} {}
    };

    struct UniformArgumentHead : Argument {
        size_t size{};
        UniformArgumentHead() noexcept : Argument{Tag::UNIFORM} {}
        explicit UniformArgumentHead(size_t size) noexcept
            : Argument{Tag::UNIFORM}, size{size} {}
    };

    struct UniformArgument : UniformArgumentHead {
        const std::byte *data{};
        UniformArgument(UniformArgumentHead head, const std::byte *data) noexcept
            : UniformArgumentHead{head}, data{data} {}
        [[nodiscard]] auto span() const noexcept { return luisa::span{data, size}; }
    };

    struct BindlessArrayArgument : Argument {
        uint64_t handle{};
        BindlessArrayArgument() noexcept : Argument{Tag::BINDLESS_ARRAY} {}
        explicit BindlessArrayArgument(uint64_t handle) noexcept
            : Argument{Tag::BINDLESS_ARRAY}, handle{handle} {}
    };

    struct AccelArgument : Argument {
        uint64_t handle{};
        AccelArgument() noexcept : Argument{Tag::ACCEL} {}
        explicit AccelArgument(uint64_t handle) noexcept
            : Argument{Tag::ACCEL}, handle{handle} {}
    };

private:
    uint64_t _handle{};
    Function _kernel{};
    size_t _argument_buffer_size{0u};
    uint _dispatch_size[3]{};
    uint32_t _argument_count{0u};
    luisa::vector<std::byte> _argument_buffer;

private:
    ShaderDispatchCommand() noexcept
        : Command{Command::Tag::EShaderDispatchCommand} {}

private:
    void _encode_pending_bindings() noexcept;
    void _encode_buffer(uint64_t handle, size_t offset, size_t size) noexcept;
    void _encode_texture(uint64_t handle, uint32_t level) noexcept;
    void _encode_uniform(const void *data, size_t size) noexcept;
    void _encode_bindless_array(uint64_t handle) noexcept;
    void _encode_accel(uint64_t handle) noexcept;
    [[nodiscard]] std::byte *_make_space(size_t size) noexcept;

    template<typename T>
        requires std::is_base_of_v<Argument, T> &&
                 std::negation_v<std::is_same<T, Argument>>
    void _encode_argument(T argument) noexcept {
        auto p = _make_space(sizeof(T));
        std::memcpy(p, &argument, sizeof(T));
        _argument_count++;
    }

public:
    explicit ShaderDispatchCommand(uint64_t handle, Function kernel) noexcept;
    ShaderDispatchCommand(ShaderDispatchCommand &&) noexcept = default;
    ShaderDispatchCommand &operator=(ShaderDispatchCommand &&) noexcept = default;
    void set_dispatch_size(uint3 launch_size) noexcept;
    [[nodiscard]] auto handle() const noexcept { return _handle; }
    [[nodiscard]] auto kernel() const noexcept { return _kernel; }
    [[nodiscard]] auto argument_count() const noexcept { return static_cast<size_t>(_argument_count); }
    [[nodiscard]] auto dispatch_size() const noexcept { return uint3(_dispatch_size[0], _dispatch_size[1], _dispatch_size[2]); }

    void encode_buffer(uint64_t handle, size_t offset, size_t size) noexcept;
    void encode_texture(uint64_t handle, uint32_t level) noexcept;
    void encode_uniform(const void *data, size_t size) noexcept;
    void encode_bindless_array(uint64_t handle) noexcept;
    void encode_accel(uint64_t handle) noexcept;

    template<typename Visit>
    void decode(Visit &&visit) const noexcept {
        auto p = _argument_buffer.data();
        auto end = _argument_buffer.data() + _argument_buffer.size();
        while (p < end) {
            Argument argument{};
            std::memcpy(&argument, p, sizeof(Argument));
            switch (argument.tag) {
                case Argument::Tag::BUFFER: {
                    BufferArgument buffer_argument{};
                    std::memcpy(&buffer_argument, p, sizeof(BufferArgument));
                    visit(buffer_argument);
                    p += sizeof(BufferArgument);
                    break;
                }
                case Argument::Tag::TEXTURE: {
                    TextureArgument texture_argument{};
                    std::memcpy(&texture_argument, p, sizeof(TextureArgument));
                    visit(texture_argument);
                    p += sizeof(TextureArgument);
                    break;
                }
                case Argument::Tag::UNIFORM: {
                    UniformArgumentHead head{};
                    std::memcpy(&head, p, sizeof(UniformArgumentHead));
                    p += sizeof(UniformArgumentHead);
                    visit(UniformArgument{head, p});
                    p += head.size;
                    break;
                }
                case Argument::Tag::BINDLESS_ARRAY: {
                    BindlessArrayArgument bindless_array_argument;
                    std::memcpy(&bindless_array_argument, p, sizeof(BindlessArrayArgument));
                    visit(bindless_array_argument);
                    p += sizeof(BindlessArrayArgument);
                    break;
                }
                case Argument::Tag::ACCEL: {
                    AccelArgument accel_argument;
                    std::memcpy(&accel_argument, p, sizeof(AccelArgument));
                    visit(accel_argument);
                    p += sizeof(AccelArgument);
                    break;
                }
                default: {
                    LUISA_ERROR_WITH_LOCATION("Invalid argument.");
                    break;
                }
            }
        }
    }
    LUISA_MAKE_COMMAND_COMMON(ShaderDispatchCommand)
};

// TODO: allow compaction/update
enum struct AccelUsageHint : uint32_t {
    FAST_TRACE,// build with best quality
    FAST_BUILD // optimize for frequent rebuild, maybe without compaction
};

enum struct AccelBuildRequest : uint32_t {
    PREFER_UPDATE,
    FORCE_BUILD,
};

class LC_RUNTIME_API MeshBuildCommand final : public Command {

private:
    uint64_t _handle{};
    AccelBuildRequest _request{};
    uint64_t _vertex_buffer{};
    size_t _vertex_buffer_offset{};
    size_t _vertex_buffer_size{};
    size_t _vertex_stride{};
    uint64_t _triangle_buffer{};
    size_t _triangle_buffer_offset{};
    size_t _triangle_buffer_size{};

private:
    MeshBuildCommand() noexcept
        : Command{Command::Tag::EMeshBuildCommand} {}

public:
    MeshBuildCommand(uint64_t handle, AccelBuildRequest request, uint64_t vertex_buffer,
                     size_t vertex_buffer_offset, size_t vertex_buffer_size, size_t vertex_stride,
                     uint64_t triangle_buffer, size_t triangle_buffer_offset, size_t triangle_buffer_size) noexcept
        : Command{Command::Tag::EMeshBuildCommand}, _handle{handle}, _request{request},
          _vertex_buffer{vertex_buffer}, _vertex_buffer_offset{vertex_buffer_offset},
          _vertex_buffer_size{vertex_buffer_size}, _vertex_stride{vertex_stride},
          _triangle_buffer{triangle_buffer}, _triangle_buffer_offset{triangle_buffer_offset},
          _triangle_buffer_size{triangle_buffer_size} {}
    [[nodiscard]] auto handle() const noexcept { return _handle; }
    [[nodiscard]] auto vertex_stride() const noexcept { return _vertex_stride; }
    [[nodiscard]] auto request() const noexcept { return _request; }
    [[nodiscard]] auto vertex_buffer() const noexcept { return _vertex_buffer; }
    [[nodiscard]] auto triangle_buffer() const noexcept { return _triangle_buffer; }
    [[nodiscard]] auto vertex_buffer_offset() const noexcept { return _vertex_buffer_offset; }
    [[nodiscard]] auto vertex_buffer_size() const noexcept { return _vertex_buffer_size; }
    [[nodiscard]] auto triangle_buffer_offset() const noexcept { return _triangle_buffer_offset; }
    [[nodiscard]] auto triangle_buffer_size() const noexcept { return _triangle_buffer_size; }
    LUISA_MAKE_COMMAND_COMMON(MeshBuildCommand)
};

class LC_RUNTIME_API AccelBuildCommand final : public Command {
    friend class CmdSer;
    friend class CmdDeser;

public:
    struct alignas(16) Modification {

        // flags
        static constexpr auto flag_mesh = 1u << 0u;
        static constexpr auto flag_transform = 1u << 1u;
        static constexpr auto flag_visibility_on = 1u << 2u;
        static constexpr auto flag_visibility_off = 1u << 3u;
        static constexpr auto flag_visibility = flag_visibility_on | flag_visibility_off;

        // members
        uint index{};
        uint flags{};
        uint64_t mesh{};
        float affine[12]{};

        // ctor
        Modification() noexcept = default;
        explicit Modification(uint index) noexcept : index{index} {}

        // encode interfaces
        void set_transform(float4x4 m) noexcept;
        void set_visibility(bool vis) noexcept;
        void set_mesh(uint64_t handle) noexcept;
    };

private:
    uint64_t _handle;
    uint32_t _instance_count;
    AccelBuildRequest _request;
    luisa::vector<Modification> _modifications;

public:
    AccelBuildCommand(uint64_t handle, uint32_t instance_count,
                      AccelBuildRequest request, luisa::vector<Modification> modifications) noexcept
        : Command{Command::Tag::EAccelBuildCommand},
          _handle{handle}, _instance_count{instance_count},
          _request{request}, _modifications{std::move(modifications)} {}
    [[nodiscard]] auto handle() const noexcept { return _handle; }
    [[nodiscard]] auto request() const noexcept { return _request; }
    [[nodiscard]] auto instance_count() const noexcept { return _instance_count; }
    [[nodiscard]] auto modifications() const noexcept { return luisa::span{_modifications}; }
    LUISA_MAKE_COMMAND_COMMON(AccelBuildCommand)
};

class LC_RUNTIME_API BindlessArrayUpdateCommand final : public Command {

private:
    uint64_t _handle;

public:
    explicit BindlessArrayUpdateCommand(uint64_t handle) noexcept
        : Command{Command::Tag::EBindlessArrayUpdateCommand}, _handle{handle} {}
    [[nodiscard]] auto handle() const noexcept { return _handle; }
    LUISA_MAKE_COMMAND_COMMON(BindlessArrayUpdateCommand)
};

#undef LUISA_MAKE_COMMAND_COMMON_CREATE
#undef LUISA_MAKE_COMMAND_COMMON_ACCEPT
#undef LUISA_MAKE_COMMAND_COMMON_RECYCLE
#undef LUISA_MAKE_COMMAND_COMMON

}// namespace luisa::compute
