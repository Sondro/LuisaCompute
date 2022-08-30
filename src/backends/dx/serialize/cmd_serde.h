#pragma once
#include <runtime/command.h>
#include <runtime/command_buffer.h>
#include <vstl/Common.h>
namespace luisa::compute {
class CmdSerde {
public:
	struct Serializer {
		// Serialized bytes
		luisa::vector<std::byte> bytes;
		// require readback pointers
		luisa::vector<std::pair<void*, size_t>> readbackSpan;
		size_t uploadSize;
		template<typename T>
			requires(std::is_trivial_v<T> && !std::is_pointer_v<T>)
		Serializer& operator<<(T const& d);
		Serializer& operator<<(luisa::span<std::byte const> data);
	};
	struct DeSerializer {
		// upload cache data
		vstd::vector<std::byte> uploadDatas;
		// readback cache data;
		vstd::vector<std::byte> readbackDatas;
		std::byte const* ptr;
		std::byte* uploadPtr;
		std::byte* readbackPtr;
		template<typename T>
			requires(std::is_trivial_v<T> && !std::is_pointer_v<T>)
		DeSerializer& operator>>(T& d);
		DeSerializer& operator>>(luisa::span<std::byte> data);
		std::byte* DeserUploadData(size_t size);
		std::byte* DeserDownloadData(size_t size);
	};
	Serializer arr;
	DeSerializer ptr;
	vstd::HashMap<uint64, luisa::shared_ptr<detail::FunctionBuilder>> functions;

private:
	void SerCmdType(BufferUploadCommand const* cmd);
	void SerCmdType(BufferDownloadCommand const* cmd);
	void SerCmdType(BufferCopyCommand const* cmd);
	void SerCmdType(BufferToTextureCopyCommand const* cmd);
	void SerCmdType(ShaderDispatchCommand const* cmd);
	void SerCmdType(TextureUploadCommand const* cmd);
	void SerCmdType(TextureDownloadCommand const* cmd);
	void SerCmdType(TextureCopyCommand const* cmd);
	void SerCmdType(TextureToBufferCopyCommand const* cmd);
	void SerCmdType(AccelBuildCommand const* cmd);
	void SerCmdType(MeshBuildCommand const* cmd);
	void SerCmdType(BindlessArrayUpdateCommand const* cmd);
	void SerType(Command const* cmd);

	void DeserCmdType(BufferUploadCommand* cmd);
	void DeserCmdType(BufferDownloadCommand* cmd);
	void DeserCmdType(BufferCopyCommand* cmd);
	void DeserCmdType(BufferToTextureCopyCommand* cmd);
	void DeserCmdType(ShaderDispatchCommand* cmd);
	void DeserCmdType(TextureUploadCommand* cmd);
	void DeserCmdType(TextureDownloadCommand* cmd);
	void DeserCmdType(TextureCopyCommand* cmd);
	void DeserCmdType(TextureToBufferCopyCommand* cmd);
	void DeserCmdType(AccelBuildCommand* cmd);
	void DeserCmdType(MeshBuildCommand* cmd);
	void DeserCmdType(BindlessArrayUpdateCommand* cmd);

public:
	void SerCommands(CommandBuffer const& cmds);
	void DeserCommands(CommandBuffer& cmds);
};
}// namespace luisa::compute