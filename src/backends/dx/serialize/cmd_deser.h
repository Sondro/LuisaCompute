#pragma once
#include <runtime/command.h>
#include <runtime/command_list.h>
#include <vstl/Common.h>
#include "array_iostream.h"
namespace luisa::compute {
class CmdDeser : public vstd::IOperatorNewBase {
public:
	vstd::vector<std::byte> uploadDatas;
	// readback cache data;
	vstd::vector<std::byte> readbackDatas;
	ArrayOStream* arr = nullptr;

	std::byte* DeserUploadData(size_t size);
	std::byte* DeserDownloadData(size_t size);

private:
	std::byte* uploadPtr;
	std::byte* readbackPtr;
	luisa::move_only_function<Function(uint64 hash)> getFunc;

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
	CmdDeser(
		luisa::move_only_function<Function(uint64 hash)>&& getFunc);
	~CmdDeser();
	void DeserCommands(CommandList& cmds);
};
}// namespace luisa::compute