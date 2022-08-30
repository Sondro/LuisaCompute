#include "cmd_serde.h"
namespace luisa::compute {
template<typename T>
	requires(std::is_trivial_v<T> && !std::is_pointer_v<T>)
CmdSerde::DeSerializer& CmdSerde::DeSerializer::operator>>(T& d) {
	memcpy(&d, ptr, sizeof(T));
	ptr += sizeof(T);
	return *this;
}
CmdSerde::DeSerializer& CmdSerde::DeSerializer::operator>>(vstd::span<std::byte> data) {
	memcpy(data.data(), ptr, data.size());
	ptr += data.size();
	return *this;
}
std::byte* CmdSerde::DeSerializer::DeserUploadData(size_t size) {
	auto ptr = uploadPtr;
	uploadPtr += size;
	(*this) >> vstd::span<std::byte>(ptr, size);
	return ptr;
}
std::byte* CmdSerde::DeSerializer::DeserDownloadData(size_t size) {
	auto data = readbackPtr;
	readbackPtr += size;
	return data;
}
void CmdSerde::DeserCmdType(BufferUploadCommand* cmd) {
	ptr >> cmd->_handle >> cmd->_offset >> cmd->_size;
	cmd->_data = ptr.DeserUploadData(cmd->_size);
}
void CmdSerde::DeserCmdType(BufferDownloadCommand* cmd) {
	ptr >> cmd->_handle >> cmd->_offset >> cmd->_size;
	cmd->_data = ptr.DeserDownloadData(cmd->_size);
}
void CmdSerde::DeserCmdType(BufferCopyCommand* cmd) {
	ptr >> (cmd->_src_handle) >> (cmd->_dst_handle) >> (cmd->_src_offset) >> (cmd->_dst_offset) >> (cmd->_size);
}
void CmdSerde::DeserCmdType(BufferToTextureCopyCommand* cmd) {
	ptr >> (cmd->_buffer_handle) >> (cmd->_buffer_offset) >> (cmd->_texture_handle) >> (cmd->_pixel_storage) >> (cmd->_texture_level) >> (cmd->_texture_size[0]) >> (cmd->_texture_size[1]) >> (cmd->_texture_size[2]);
}
void CmdSerde::DeserCmdType(ShaderDispatchCommand* cmd) {
	uint64 funcHash;
	ptr >> (cmd->_handle) >> funcHash >> (cmd->_dispatch_size[0]) >> (cmd->_dispatch_size[1]) >> (cmd->_dispatch_size[2]) >> (cmd->_argument_count);
	auto ite = functions.Find(funcHash);
	assert(ite);
	cmd->_kernel = Function{ite.Value().get()};
}
void CmdSerde::DeserCmdType(TextureUploadCommand* cmd) {
	ptr >> (cmd->_handle) >> (cmd->_storage) >> (cmd->_level) >> (cmd->_size[0]) >> (cmd->_size[1]) >> (cmd->_size[2]);
	auto byteSize = pixel_storage_size(
		cmd->_storage,
		cmd->_size[0],
		cmd->_size[1],
		cmd->_size[2]);
	ptr.DeserUploadData(byteSize);
}
void CmdSerde::DeserCmdType(TextureDownloadCommand* cmd) {
	ptr >> (cmd->_handle) >> (cmd->_storage) >> (cmd->_level) >> (cmd->_size[0]) >> (cmd->_size[1]) >> (cmd->_size[2]);
	auto byteSize = pixel_storage_size(
		cmd->_storage,
		cmd->_size[0],
		cmd->_size[1],
		cmd->_size[2]);
	cmd->_data = ptr.DeserDownloadData(byteSize);
}
void CmdSerde::DeserCmdType(TextureCopyCommand* cmd) {
	ptr >> (cmd->_storage) >> (cmd->_src_handle) >> (cmd->_dst_handle) >> (cmd->_size[0]) >> (cmd->_size[1]) >> (cmd->_size[2]) >> (cmd->_src_level) >> (cmd->_dst_level);
}
void CmdSerde::DeserCmdType(TextureToBufferCopyCommand* cmd) {
	ptr >> (cmd->_buffer_handle) >> (cmd->_buffer_offset) >> (cmd->_texture_handle) >> (cmd->_pixel_storage) >> (cmd->_texture_level) >> (cmd->_texture_size[0]) >> (cmd->_texture_size[1]) >> (cmd->_texture_size[2]);
}
void CmdSerde::DeserCmdType(AccelBuildCommand* cmd) {
	size_t modSize;
	ptr >> (cmd->_handle) >> (cmd->_instance_count) >> (cmd->_request) >> modSize;
	cmd->_modifications.resize(modSize);
	for (auto&& i : cmd->_modifications) {
		ptr >> i.index >> i.flags >> i.mesh;
		for (auto&& j : i.affine) {
			ptr >> j;
		}
	}
}
void CmdSerde::DeserCmdType(MeshBuildCommand* cmd) {
	ptr >> (cmd->_handle) >> (cmd->_request) >> (cmd->_vertex_buffer) >> (cmd->_vertex_stride) >> (cmd->_vertex_buffer_offset) >> (cmd->_vertex_buffer_size) >> (cmd->_triangle_buffer) >> (cmd->_triangle_buffer_offset) >> (cmd->_triangle_buffer_size);
}
void CmdSerde::DeserCmdType(BindlessArrayUpdateCommand* cmd) {
	ptr >> (cmd->_handle);
}
void CmdSerde::DeserCommands(CommandBuffer& cmds) {
	size_t cmdSize;
	size_t readbackSize;
	size_t uploadSize;
	auto&& vec = cmds._command_list._commands;
	ptr >> cmdSize >> readbackSize >> uploadSize;
	ptr.readbackDatas.clear();
	ptr.uploadDatas.clear();
	ptr.readbackDatas.resize(readbackSize);
	ptr.uploadDatas.resize(uploadSize);
	ptr.readbackPtr = ptr.readbackDatas.data();
	ptr.uploadPtr = ptr.uploadDatas.data();
	vec.resize(cmdSize);
	Command::Tag tag;
	for (auto&& i : vec) {
		ptr >> tag;
		switch (tag) {
			case Command::Tag::EBufferUploadCommand: {
				DeserCmdType(static_cast<BufferUploadCommand*>(i));
			} break;
			case Command::Tag::EBufferDownloadCommand: {
				DeserCmdType(static_cast<BufferDownloadCommand*>(i));
			} break;
			case Command::Tag::EBufferToTextureCopyCommand: {
				DeserCmdType(static_cast<BufferToTextureCopyCommand*>(i));
			} break;
			case Command::Tag::EShaderDispatchCommand: {
				DeserCmdType(static_cast<ShaderDispatchCommand*>(i));
			} break;
			case Command::Tag::ETextureUploadCommand: {
				DeserCmdType(static_cast<TextureUploadCommand*>(i));
			} break;
			case Command::Tag::ETextureDownloadCommand: {
				DeserCmdType(static_cast<TextureDownloadCommand*>(i));
			} break;
			case Command::Tag::ETextureCopyCommand: {
				DeserCmdType(static_cast<TextureCopyCommand*>(i));
			} break;
			case Command::Tag::ETextureToBufferCopyCommand: {
				DeserCmdType(static_cast<TextureToBufferCopyCommand*>(i));
			} break;
			case Command::Tag::EAccelBuildCommand: {
				DeserCmdType(static_cast<AccelBuildCommand*>(i));
			} break;
			case Command::Tag::EMeshBuildCommand: {
				DeserCmdType(static_cast<MeshBuildCommand*>(i));
			} break;
			case Command::Tag::EBindlessArrayUpdateCommand: {
				DeserCmdType(static_cast<BindlessArrayUpdateCommand*>(i));
			} break;
			default:
				LUISA_ERROR("Illegal Command!");
				break;
		}
	}
}
}// namespace luisa::compute