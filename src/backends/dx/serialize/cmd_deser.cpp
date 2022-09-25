#include "cmd_deser.h"
namespace luisa::compute {
std::byte const* CmdDeser::DeserUploadData(size_t size) {
	auto ptr = arr->ptr;
	arr->ptr += size;
	return ptr;
}
std::byte* CmdDeser::DeserDownloadData(size_t size) {
	auto data = readbackPtr;
	readbackPtr += size;
	return data;
}
void CmdDeser::DeserCmdType(BufferUploadCommand* cmd) {
	*arr >> cmd->_handle >> cmd->_offset >> cmd->_size;
	cmd->_handle = visitor->GetResource(cmd->_handle);
	cmd->_data = DeserUploadData(cmd->_size);
}
void CmdDeser::DeserCmdType(BufferDownloadCommand* cmd) {
	*arr >> cmd->_handle >> cmd->_offset >> cmd->_size;
	cmd->_handle = visitor->GetResource(cmd->_handle);
	cmd->_data = DeserDownloadData(cmd->_size);
}
void CmdDeser::DeserCmdType(BufferCopyCommand* cmd) {
	*arr >> (cmd->_src_handle) >> (cmd->_dst_handle) >> (cmd->_src_offset) >> (cmd->_dst_offset) >> (cmd->_size);
	cmd->_src_handle = visitor->GetResource(cmd->_src_handle);
	cmd->_dst_handle = visitor->GetResource(cmd->_dst_handle);
}
void CmdDeser::DeserCmdType(BufferToTextureCopyCommand* cmd) {
	*arr >> (cmd->_buffer_handle) >> (cmd->_buffer_offset) >> (cmd->_texture_handle) >> (cmd->_pixel_storage) >> (cmd->_texture_level) >> (cmd->_texture_size[0]) >> (cmd->_texture_size[1]) >> (cmd->_texture_size[2]);
	cmd->_buffer_handle = visitor->GetResource(cmd->_buffer_handle);
	cmd->_texture_handle = visitor->GetResource(cmd->_texture_handle);
}
void CmdDeser::DeserCmdType(ShaderDispatchCommand* cmd) {
	uint64 funcHash;
	*arr >> (cmd->_handle) >> funcHash >> (cmd->_dispatch_size[0]) >> (cmd->_dispatch_size[1]) >> (cmd->_dispatch_size[2]) >> (cmd->_argument_count);
	cmd->_kernel = visitor->GetFunction(funcHash);
}
void CmdDeser::DeserCmdType(TextureUploadCommand* cmd) {
	*arr >> (cmd->_handle) >> (cmd->_storage) >> (cmd->_level) >> (cmd->_size[0]) >> (cmd->_size[1]) >> (cmd->_size[2]);
	cmd->_handle = visitor->GetResource(cmd->_handle);
	auto byteSize = pixel_storage_size(
		cmd->_storage,
		cmd->_size[0],
		cmd->_size[1],
		cmd->_size[2]);
	DeserUploadData(byteSize);
}
void CmdDeser::DeserCmdType(TextureDownloadCommand* cmd) {
	*arr >> (cmd->_handle) >> (cmd->_storage) >> (cmd->_level) >> (cmd->_size[0]) >> (cmd->_size[1]) >> (cmd->_size[2]);
	cmd->_handle = visitor->GetResource(cmd->_handle);
	auto byteSize = pixel_storage_size(
		cmd->_storage,
		cmd->_size[0],
		cmd->_size[1],
		cmd->_size[2]);
	cmd->_data = DeserDownloadData(byteSize);
}
void CmdDeser::DeserCmdType(TextureCopyCommand* cmd) {
	*arr >> (cmd->_storage) >> (cmd->_src_handle) >> (cmd->_dst_handle) >> (cmd->_size[0]) >> (cmd->_size[1]) >> (cmd->_size[2]) >> (cmd->_src_level) >> (cmd->_dst_level);
	cmd->_src_handle = visitor->GetResource(cmd->_src_handle);
	cmd->_dst_handle = visitor->GetResource(cmd->_dst_handle);
}
void CmdDeser::DeserCmdType(TextureToBufferCopyCommand* cmd) {
	*arr >> (cmd->_buffer_handle) >> (cmd->_buffer_offset) >> (cmd->_texture_handle) >> (cmd->_pixel_storage) >> (cmd->_texture_level) >> (cmd->_texture_size[0]) >> (cmd->_texture_size[1]) >> (cmd->_texture_size[2]);
	cmd->_buffer_handle = visitor->GetResource(cmd->_buffer_handle);
	cmd->_texture_handle = visitor->GetResource(cmd->_texture_handle);
}
void CmdDeser::DeserCmdType(AccelBuildCommand* cmd) {
	size_t modSize;
	*arr >> (cmd->_handle) >> (cmd->_instance_count) >> (cmd->_request) >> modSize;
	cmd->_handle = visitor->GetResource(cmd->_handle);
	cmd->_modifications.resize(modSize);
	for (auto&& i : cmd->_modifications) {
		*arr >> i.index >> i.flags >> i.mesh;
		i.mesh = visitor->GetResource(i.mesh);
		for (auto&& j : i.affine) {
			*arr >> j;
		}
	}
}
void CmdDeser::DeserCmdType(MeshBuildCommand* cmd) {
	*arr >> (cmd->_handle) >> (cmd->_request) >> (cmd->_vertex_buffer) >> (cmd->_vertex_stride) >> (cmd->_vertex_buffer_offset) >> (cmd->_vertex_buffer_size) >> (cmd->_triangle_buffer) >> (cmd->_triangle_buffer_offset) >> (cmd->_triangle_buffer_size);
	cmd->_handle = visitor->GetResource(cmd->_handle);
	cmd->_vertex_buffer = visitor->GetResource(cmd->_vertex_buffer);
	cmd->_triangle_buffer = visitor->GetResource(cmd->_triangle_buffer);
}
void CmdDeser::DeserCmdType(BindlessArrayUpdateCommand* cmd) {
	*arr >> (cmd->_handle);
	cmd->_handle = visitor->GetResource(cmd->_handle);
}
void CmdDeser::DeserCommands(size_t headerSize, CommandList& cmds) {
	size_t cmdSize;
	size_t readbackSize;
	size_t uploadSize;
	auto&& vec = cmds._commands;
	*arr >> cmdSize >> readbackSize >> uploadSize;
	readbackDatas.resize(headerSize + sizeof(size_t) + readbackSize);
	memcpy(readbackDatas.data() + headerSize, &readbackSize, sizeof(size_t));
	readbackPtr = readbackDatas.data() + headerSize;
	vec.resize(cmdSize);
	Command::Tag tag;
	for (auto&& i : vec) {
		*arr >> tag;
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
CmdDeser::CmdDeser() {}
CmdDeser::~CmdDeser() {}
}// namespace luisa::compute