#pragma once
#include <runtime/command.h>
#include <runtime/command_list.h>
#include <vstl/Common.h>
#include "array_iostream.h"
namespace luisa::compute {
class CmdDeser : public vstd::IOperatorNewBase {
public:
	class IHandleMapVisitor {
	protected:
		~IHandleMapVisitor() = default;

	public:
		virtual uint64 GetResource(uint64 handle) = 0;
		virtual Function GetFunction(uint64 handle) = 0;
	};
	// readback cache data;
	luisa::vector<std::byte> readbackDatas;
	IHandleMapVisitor* visitor = nullptr;
	ArrayOStream* arr = nullptr;

	std::byte const* DeserUploadData(size_t size);
	std::byte* DeserDownloadData(size_t size);

private:
	std::byte* readbackPtr;

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
	CmdDeser();
	~CmdDeser();
	void DeserCommands(size_t headerSize, CommandList& cmds);
};
}// namespace luisa::compute