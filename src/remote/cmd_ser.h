#pragma once
#include <runtime/command.h>
#include <runtime/command_list.h>
#include <vstl/Common.h>
#include "array_iostream.h"
namespace luisa::compute {
class CmdSer : public vstd::IOperatorNewBase {
public:
	// Serialized bytes
	ArrayIStream* arr = nullptr;
	// require readback pointers
	luisa::vector<std::pair<void*, size_t>> readbackSpan;
	size_t uploadSize;

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

public:
	void SerCommands(CommandList const& cmds);
	CmdSer();
	~CmdSer();
};
}// namespace luisa::compute