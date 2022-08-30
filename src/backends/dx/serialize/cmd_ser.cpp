#include "cmd_serde.h"
namespace luisa::compute {
template<typename T>
	requires(std::is_trivial_v<T> && !std::is_pointer_v<T>)
CmdSerde::Serializer& CmdSerde::Serializer::operator<<(T const& d) {
	auto idx = bytes.size();
	bytes.resize(idx + sizeof(T));
	memcpy(bytes.data() + idx, &d, sizeof(T));
	return *this;
}
CmdSerde::Serializer& CmdSerde::Serializer::operator<<(vstd::span<std::byte const> data) {
	auto idx = bytes.size();
	bytes.resize(idx + data.size_bytes());
	memcpy(bytes.data() + idx, data.data(), data.size_bytes());
	return *this;
}
void CmdSerde::SerCmdType(BufferUploadCommand const* cmd) {
	arr << (cmd->_handle) << (cmd->_offset) << (cmd->_size) << vstd::span<std::byte const>{reinterpret_cast<std::byte const*>(cmd->_data), cmd->_size};
	arr.uploadSize += cmd->_size;
}
void CmdSerde::SerCmdType(BufferDownloadCommand const* cmd) {
	arr << (cmd->_handle) << (cmd->_offset) << (cmd->_size);
	arr.readbackSpan.emplace_back(cmd->_data, cmd->_size);
}
void CmdSerde::SerCmdType(BufferCopyCommand const* cmd) {
	arr << (cmd->_src_handle) << (cmd->_dst_handle) << (cmd->_src_offset) << (cmd->_dst_offset) << (cmd->_size);
}
void CmdSerde::SerCmdType(BufferToTextureCopyCommand const* cmd) {
	arr << (cmd->_buffer_handle) << (cmd->_buffer_offset) << (cmd->_texture_handle) << (cmd->_pixel_storage) << (cmd->_texture_level) << (cmd->_texture_size[0]) << (cmd->_texture_size[1]) << (cmd->_texture_size[2]);
}
void CmdSerde::SerCmdType(TextureToBufferCopyCommand const* cmd) {
	arr << (cmd->_buffer_handle) << (cmd->_buffer_offset) << (cmd->_texture_handle) << (cmd->_pixel_storage) << (cmd->_texture_level) << (cmd->_texture_size[0]) << (cmd->_texture_size[1]) << (cmd->_texture_size[2]);
}
void CmdSerde::SerCmdType(TextureCopyCommand const* cmd) {
	arr << (cmd->_storage) << (cmd->_src_handle) << (cmd->_dst_handle) << (cmd->_size[0]) << (cmd->_size[1]) << (cmd->_size[2]) << (cmd->_src_level) << (cmd->_dst_level);
}
void CmdSerde::SerCmdType(TextureUploadCommand const* cmd) {
	auto byteSize = pixel_storage_size(
		cmd->_storage,
		cmd->_size[0],
		cmd->_size[1],
		cmd->_size[2]);
	arr << (cmd->_handle) << (cmd->_storage) << (cmd->_level) << (cmd->_size[0]) << (cmd->_size[1]) << (cmd->_size[2]) << vstd::span<std::byte const>{reinterpret_cast<std::byte const*>(cmd->_data), byteSize};

}
void CmdSerde::SerCmdType(TextureDownloadCommand const* cmd) {
	arr << (cmd->_handle) << (cmd->_storage) << (cmd->_level) << (cmd->_size[0]) << (cmd->_size[1]) << (cmd->_size[2]);
	auto byteSize = pixel_storage_size(
		cmd->_storage,
		cmd->_size[0],
		cmd->_size[1],
		cmd->_size[2]);
	arr.readbackSpan.emplace_back(cmd->_data, byteSize);
	arr.uploadSize += byteSize;
}

void CmdSerde::SerCmdType(ShaderDispatchCommand const* cmd) {
	arr << (cmd->_handle) << (cmd->_kernel.hash()) << (cmd->_dispatch_size[0]) << (cmd->_dispatch_size[1]) << (cmd->_dispatch_size[2]) << (cmd->_argument_count) << vstd::span<std::byte const>{reinterpret_cast<std::byte const*>(cmd->_argument_buffer.data()), cmd->_argument_buffer.size()};
}

void CmdSerde::SerCmdType(AccelBuildCommand const* cmd) {
	arr << (cmd->_handle) << (cmd->_instance_count) << (cmd->_request) << (cmd->_modifications.size());
	for (auto&& i : cmd->_modifications) {
		arr << (i.index) << (i.flags) << (i.mesh);
		for (auto&& j : i.affine) {
			arr << (j);
		}
	}
}
void CmdSerde::SerCmdType(MeshBuildCommand const* cmd) {
	arr << (cmd->_handle) << (cmd->_request) << (cmd->_vertex_buffer) << (cmd->_vertex_stride) << (cmd->_vertex_buffer_offset) << (cmd->_vertex_buffer_size) << (cmd->_triangle_buffer) << (cmd->_triangle_buffer_offset) << (cmd->_triangle_buffer_size);
}
void CmdSerde::SerCmdType(BindlessArrayUpdateCommand const* cmd) {
	arr << (cmd->_handle);
}
void CmdSerde::SerType(Command const* cmd) {
	arr << cmd->tag();
	switch (cmd->tag()) {
		case Command::Tag::EBufferUploadCommand: {
			SerCmdType(static_cast<BufferUploadCommand const*>(cmd));
		} break;
		case Command::Tag::EBufferDownloadCommand: {
			SerCmdType(static_cast<BufferDownloadCommand const*>(cmd));
		} break;
		case Command::Tag::EBufferToTextureCopyCommand: {
			SerCmdType(static_cast<BufferToTextureCopyCommand const*>(cmd));
		} break;
		case Command::Tag::EShaderDispatchCommand: {
			SerCmdType(static_cast<ShaderDispatchCommand const*>(cmd));
		} break;
		case Command::Tag::ETextureUploadCommand: {
			SerCmdType(static_cast<TextureUploadCommand const*>(cmd));
		} break;
		case Command::Tag::ETextureDownloadCommand: {
			SerCmdType(static_cast<TextureDownloadCommand const*>(cmd));
		} break;
		case Command::Tag::ETextureCopyCommand: {
			SerCmdType(static_cast<TextureCopyCommand const*>(cmd));
		} break;
		case Command::Tag::ETextureToBufferCopyCommand: {
			SerCmdType(static_cast<TextureToBufferCopyCommand const*>(cmd));
		} break;
		case Command::Tag::EAccelBuildCommand: {
			SerCmdType(static_cast<AccelBuildCommand const*>(cmd));
		} break;
		case Command::Tag::EMeshBuildCommand: {
			SerCmdType(static_cast<MeshBuildCommand const*>(cmd));
		} break;
		case Command::Tag::EBindlessArrayUpdateCommand: {
			SerCmdType(static_cast<BindlessArrayUpdateCommand const*>(cmd));
		} break;
		default:
			LUISA_ERROR("Illegal Command!");
			break;
	}
}

void CmdSerde::SerCommands(CommandBuffer const& cmds) {
	arr.uploadSize = 0;
	auto&& list = cmds._command_list._commands;
	arr << list.size();
	auto readbackIndex = arr.bytes.size();
	arr.bytes.resize(readbackIndex + sizeof(size_t) * 2);
	for (auto&& i : list) {
		SerType(i);
	}
	size_t readbackFullSize = 0;
	for(auto&& i : arr.readbackSpan){
		readbackFullSize += i.second;
	}
	memcpy(arr.bytes.data() + readbackIndex, &readbackFullSize, sizeof(size_t));
	memcpy(arr.bytes.data() + readbackIndex + sizeof(size_t), &arr.uploadSize, sizeof(size_t));
}
}// namespace luisa::compute