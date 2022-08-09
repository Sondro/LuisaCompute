#include "lc_stream.h"
#include <gpu_collection/bindless_array.h>
#include <shader/compute_shader.h>
#include <shader/rt_shader.h>
namespace toolhub::vk {
class LCCmdBase : public CommandVisitor {
public:
	FrameResource* frameRes;
	LCStream* stream;
	CommandBuffer* cb;
};
class LCCmdPreprocesser : public LCCmdBase {
public:
	Function func;
	void visit(BufferUploadCommand const* cmd) override {
		auto bfView = BufferView(
			reinterpret_cast<Buffer const*>(cmd->handle()),
			cmd->offset(),
			cmd->size());
		stream->StateTracker().MarkBufferWrite(
			bfView,
			BufferWriteState::Copy);
		auto upload = frameRes->AllocateUpload(cmd->size());
		upload.buffer->CopyFrom({reinterpret_cast<vbyte const*>(cmd->data()), cmd->size()}, upload.offset);
		frameRes->AddCopyCmd(
			upload.buffer,
			upload.offset,
			bfView.buffer,
			bfView.offset,
			cmd->size());
	}
	void visit(BufferDownloadCommand const* cmd) override {
		auto bfView = BufferView(
			reinterpret_cast<Buffer const*>(cmd->handle()),
			cmd->offset(),
			cmd->size());
		stream->StateTracker().MarkBufferRead(
			bfView,
			BufferReadState::ComputeOrCopy);
		auto readback = frameRes->AllocateReadback(cmd->size());
		frameRes->AddCopyCmd(
			bfView.buffer,
			bfView.offset,
			readback.buffer,
			readback.offset,
			cmd->size());
		frameRes->AddDisposeEvent([readback, ptr = cmd->data(), size = cmd->size()] {
			readback.buffer->CopyTo(
				{reinterpret_cast<vbyte*>(ptr), size},
				readback.offset);
		});
	}
	void visit(BufferCopyCommand const* cmd) override {
		auto srcBfView = BufferView(
			reinterpret_cast<Buffer const*>(cmd->src_handle()),
			cmd->src_offset(),
			cmd->size());
		auto dstBfView = BufferView(
			reinterpret_cast<Buffer const*>(cmd->dst_handle()),
			cmd->dst_offset(),
			cmd->size());
		stream->StateTracker().MarkBufferRead(
			srcBfView,
			BufferReadState::ComputeOrCopy);
		stream->StateTracker().MarkBufferWrite(
			dstBfView,
			BufferWriteState::Copy);
		frameRes->AddCopyCmd(
			srcBfView.buffer,
			srcBfView.offset,
			dstBfView.buffer,
			dstBfView.offset,
			cmd->size());
	}
	void visit(BufferToTextureCopyCommand const* cmd) override {
		auto sz = cmd->size();
		auto byteSize = pixel_storage_size(cmd->storage(), sz.x, sz.y, sz.z);
		auto bfView = BufferView(
			reinterpret_cast<Buffer const*>(cmd->buffer()),
			cmd->buffer_offset(),
			byteSize);
		stream->StateTracker()
			.MarkBufferRead(
				bfView,
				BufferReadState::ComputeOrCopy);
		auto tex = reinterpret_cast<Texture const*>(cmd->texture());
		stream->StateTracker()
			.MarkTextureWrite(
				tex,
				cmd->level(),
				TextureWriteState::Copy);
		frameRes->AddCopyCmd(
			bfView.buffer,
			bfView.offset,
			tex,
			cmd->level());
	}

	void visit(TextureUploadCommand const* cmd) override {
		auto tex =
			reinterpret_cast<Texture const*>(cmd->handle());
		stream->StateTracker()
			.MarkTextureWrite(
				tex,
				cmd->level(),
				TextureWriteState::Copy);
		auto sz = cmd->size();
		auto byteSize = pixel_storage_size(cmd->storage(), sz.x, sz.y, sz.z);
		auto upload = frameRes->AllocateUpload(
			byteSize);
		upload.buffer->CopyFrom(
			{reinterpret_cast<vbyte const*>(cmd->data()),
			 byteSize},
			upload.offset);
		frameRes->AddCopyCmd(
			upload.buffer,
			upload.offset,
			tex,
			cmd->level());
	}
	void visit(TextureDownloadCommand const* cmd) override {
		auto tex = reinterpret_cast<Texture const*>(cmd->handle());
		stream->StateTracker()
			.MarkTextureRead(
				TexView(tex, cmd->level(), 1));
		auto sz = cmd->size();
		auto byteSize = pixel_storage_size(cmd->storage(), sz.x, sz.y, sz.z);
		auto readback = frameRes->AllocateReadback(
			byteSize);
		frameRes->AddCopyCmd(
			tex,
			cmd->level(),
			1,
			readback.buffer,
			readback.offset);
		frameRes->AddDisposeEvent(
			[readback,
			 ptr = cmd->data(),
			 byteSize] {
				readback.buffer->CopyTo(
					{reinterpret_cast<vbyte*>(ptr), byteSize},
					readback.offset);
			});
	}
	void visit(TextureCopyCommand const* cmd) override {
		auto srcTexView = TexView(
			reinterpret_cast<Texture const*>(cmd->src_handle()),
			cmd->src_level(), 1);
		stream->StateTracker()
			.MarkTextureRead(srcTexView);
		auto dstTex = reinterpret_cast<Texture const*>(cmd->dst_handle());
		stream->StateTracker()
			.MarkTextureWrite(
				dstTex,
				cmd->dst_level(),
				TextureWriteState::Copy);
		frameRes->AddCopyCmd(
			srcTexView.tex,
			cmd->src_level(),
			dstTex,
			cmd->dst_level());
	}
	void visit(TextureToBufferCopyCommand const* cmd) override {
		auto sz = cmd->size();
		auto byteSize = pixel_storage_size(cmd->storage(), sz.x, sz.y, sz.z);
		auto texView = TexView(reinterpret_cast<Texture const*>(cmd->texture()), cmd->level(), 1);
		auto bufView = BufferView(
			reinterpret_cast<Buffer const*>(cmd->buffer()),
			cmd->buffer_offset(),
			byteSize);
		stream->StateTracker()
			.MarkTextureRead(texView);
		stream->StateTracker()
			.MarkBufferWrite(
				bufView,
				BufferWriteState::Copy);
		frameRes->AddCopyCmd(
			texView.tex,
			cmd->level(),
			1,
			bufView.buffer,
			bufView.offset);
	}
	void visit(AccelBuildCommand const* cmd) override {
		auto mod = cmd->modifications();
		auto accel = reinterpret_cast<Accel*>(cmd->handle());
		auto& updateInfo = stream->updateInfos.emplace_back(
			accel->Preprocess(
				stream->hostAlloc,
				cb, stream->StateTracker(), cmd->instance_count(), cmd->request() == AccelBuildRequest::PREFER_UPDATE, mod.size(), frameRes));
		if (!mod.empty()) {
			auto uploadBuffer = frameRes->AllocateUpload(
				Accel::ACCEL_INST_SIZE * mod.size());
			size_t uploadOffset = uploadBuffer.offset;
			auto iterator = vstd::RangeImpl(
				vstd::CacheEndRange(mod) | vstd::TransformRange([&](AccelBuildCommand::Modification const& v) {
					Accel::Visibility vis = Accel::Visibility::Unchange;
					if (v.flags & AccelBuildCommand::Modification::flag_visibility_on)
						vis == Accel::Visibility::On;
					else if (v.flags & AccelBuildCommand::Modification::flag_visibility_off)
						vis == Accel::Visibility::Off;
					return accel->SetInstance(
						updateInfo,
						v.index,
						reinterpret_cast<Mesh*>(v.mesh),
						v.affine,
						uploadBuffer.buffer,
						uploadOffset,
						(v.flags & AccelBuildCommand::Modification::flag_transform) != 0,
						(v.flags & AccelBuildCommand::Modification::flag_mesh) != 0, vis);
				}));

			frameRes->AddCopyCmd(
				uploadBuffer.buffer,
				accel->InstanceBuffer(),
				&iterator,
				mod.size());
		}
		accel->UpdateMesh(frameRes);
		frameRes->AddScratchSize(updateInfo.scratchSize);
	}
	void visit(MeshBuildCommand const* cmd) override {
		auto buildInfo =
			reinterpret_cast<Mesh*>(cmd->handle())
				->Preprocess(
					stream->hostAlloc,
					stream->StateTracker(),
					reinterpret_cast<Buffer const*>(cmd->vertex_buffer()),
					cmd->vertex_stride(),
					cmd->vertex_buffer_offset(),
					cmd->vertex_buffer_size(),
					reinterpret_cast<Buffer const*>(cmd->triangle_buffer()),
					cmd->triangle_buffer_offset(),
					cmd->triangle_buffer_size(),
					cmd->request() == AccelBuildRequest::PREFER_UPDATE, frameRes);
		auto& v = stream->updateInfos.emplace_back(buildInfo);
		frameRes->AddScratchSize(v.scratchSize);
	}
	void visit(BindlessArrayUpdateCommand const* cmd) override {
		reinterpret_cast<BindlessArray*>(cmd->handle())->Preprocess(frameRes, stream->StateTracker(), cb);
	}
	Variable const* vars;
	void visit(ShaderDispatchCommand const* cmd) override {
		stream->bindVec.clear();
		auto&& cbufferPos = stream->bindVec.emplace_back();
		auto cs = reinterpret_cast<IShader const*>(cmd->handle());

		func = cmd->kernel();
		vars = func.arguments().data();
		stream->uniformPack.Reset();
		uint3 kernelDispCount = cmd->dispatch_size();
		stream->uniformPack.AddValue(16, 16, {reinterpret_cast<std::byte const*>(&kernelDispCount), sizeof(uint3)});
		cmd->decode(*this);
		auto uniformData = stream->uniformPack.Data();
		auto upload = frameRes->AllocateUpload(
			uniformData.size());
		BufferView cbuffer = frameRes->AllocateDefault(
			uniformData.size(),
			stream->GetDevice()->limits.minStorageBufferOffsetAlignment);
		upload.buffer->CopyFrom(
			{reinterpret_cast<vbyte const*>(uniformData.data()),
			 uniformData.size()},
			upload.offset);
		stream->StateTracker().MarkBufferWrite(
			cbuffer,
			BufferWriteState::Copy);
		frameRes->AddCopyCmd(
			upload.buffer,
			upload.offset,
			cbuffer.buffer,
			cbuffer.offset,
			uniformData.size());
		cbufferPos = cbuffer;
		stream->sets.emplace_back(
			cb->PreprocessDispatch(
				cs->GetLayout(),
				stream->bindVec),
			cbuffer);
	}
	///////////////////// shader dispatch
	void operator()(ShaderDispatchCommand::BufferArgument const& bf) {
		Buffer const* buffer = reinterpret_cast<Buffer const*>(bf.handle);
		auto usage = func.variable_usage(bf.variable_uid);
		if (((uint)usage & (uint)Usage::WRITE) == 0) {
			stream->StateTracker().MarkBufferRead(
				BufferView(buffer, bf.offset, bf.size),
				BufferReadState::ComputeOrCopy);
		} else {
			stream->StateTracker().MarkBufferWrite(
				BufferView(buffer, bf.offset, bf.size),
				BufferWriteState::Compute);
		}
		stream->bindVec.emplace_back(BufferView(buffer, bf.offset, bf.size));
		++vars;
	}
	void operator()(ShaderDispatchCommand::TextureArgument const& tex) {
		Texture const* texture = reinterpret_cast<Texture const*>(tex.handle);
		auto usage = func.variable_usage(tex.variable_uid);
		if (((uint)usage & (uint)Usage::WRITE) == 0) {
			auto texView = TexView(texture, tex.level);
			stream->StateTracker().MarkTextureRead(
				texView);
			stream->bindVec.emplace_back(texView);
		} else {
			stream->StateTracker().MarkTextureWrite(
				texture,
				tex.level,
				TextureWriteState::Compute);
			stream->bindVec.emplace_back(TexView(texture, tex.level, 1));
		}
		++vars;
	}
	void operator()(ShaderDispatchCommand::BindlessArrayArgument const& bf) {
		BindlessArray* arr = reinterpret_cast<BindlessArray*>(bf.handle);
		stream->readArr.emplace_back(arr);
		stream->StateTracker().MarkBufferRead(
			arr->InstanceBuffer(),
			BufferReadState::ComputeOrCopy);
		stream->bindVec.emplace_back(arr->InstanceBuffer());
		++vars;
	}
	void operator()(ShaderDispatchCommand::UniformArgument const& a) {
		stream->uniformPack.AddValue(vars->type(), {a.data, a.size});
		++vars;
	}
	void operator()(ShaderDispatchCommand::AccelArgument const& bf) {
		auto accel = reinterpret_cast<Accel const*>(bf.handle);
		auto usage = func.variable_usage(bf.variable_uid);
		if (((uint)usage & (uint)Usage::WRITE) == 0) {
			stream->bindVec.emplace_back(accel);
			stream->StateTracker().MarkBufferRead(
				accel->AccelBuffer(),
				BufferReadState::UseAccel);
		} else {
			stream->bindVec.emplace_back(accel->InstanceBuffer());
			stream->StateTracker().MarkBufferWrite(
				accel->InstanceBuffer(),
				BufferWriteState::Compute);
		}
		++vars;
	}
};
class LCCmdVisitor : public LCCmdBase {
public:
	BuildInfo* buildInfo;
	Function func;
	UniformBuffer* sets;
	///////////////////// unused
	void visit(BufferUploadCommand const* cmd) override {}
	void visit(BufferDownloadCommand const* cmd) override {}
	void visit(BufferCopyCommand const* cmd) override {}
	void visit(BufferToTextureCopyCommand const* cmd) override {}
	void visit(TextureUploadCommand const* cmd) override {}
	void visit(TextureDownloadCommand const* cmd) override {}
	void visit(TextureCopyCommand const* cmd) override {}
	void visit(TextureToBufferCopyCommand const* cmd) override {}
	void visit(BindlessArrayUpdateCommand const* cmd) override {}

	///////////////////// build
	void visit(ShaderDispatchCommand const* cmd) override {
		auto ishad = reinterpret_cast<IShader const*>(cmd->handle());
		if (ishad->GetTag() == IShader::Tag::Compute) {
			cb->Dispatch(
				sets->set,
				static_cast<ComputeShader const*>(ishad),
				cmd->dispatch_size());
		} else {
			cb->Dispatch(
				sets->set,
				static_cast<RTShader const*>(ishad),
				cmd->dispatch_size());
		}
		++sets;
	}
	BuildInfo& GetNextBuildInfo() {
		auto& v = *buildInfo;
		auto scratch = frameRes->GetScratchBufferView();
		v.buffer = scratch.buffer;
		v.scratchOffset = scratch.offset;
		++buildInfo;
		return v;
	}
	void visit(AccelBuildCommand const* cmd) override {
		auto accel = reinterpret_cast<Accel*>(cmd->handle());
		accel->Build(
			stream->hostAlloc,
			stream->StateTracker(),
			cb,
			GetNextBuildInfo(),
			cmd->modifications().size(),
			cmd->instance_count(),
			stream->accelBuildCmd,
			stream->accelRangeCmd);
	}
	void visit(MeshBuildCommand const* cmd) override {
		auto mesh = reinterpret_cast<Mesh*>(cmd->handle());
		mesh->Build(
			stream->hostAlloc,
			GetNextBuildInfo(),
			cmd->triangle_buffer_size(),
			stream->accelBuildCmd,
			stream->accelRangeCmd);
		if (mesh->AllowCompact()) {
			stream->compactAccel.emplace_back(mesh->PreprocessLoadCompactSize(stream->StateTracker()));
			stream->compactMesh.emplace_back(mesh);
		}
	}
};
void LCStream::DispatchCmd(CommandList&& cmdList) {
	auto frameRes = BeginPrepareFrame();
	auto cmdBuffer = frameRes->GetCmdBuffer();
	LCCmdPreprocesser prep;
	LCCmdVisitor visitor;
	prep.stream = this;
	prep.frameRes = frameRes;
	prep.cb = cmdBuffer;
	visitor.stream = this;
	visitor.frameRes = frameRes;
	visitor.cb = cmdBuffer;
	auto commands = cmdList.steal_commands();
	for (auto command : commands) {
		command->accept(reorder);
	}
	auto cmdLists = reorder.command_lists();
	auto AfterFunc = vstd::create_disposer([&] {
		for (auto command : commands) {
			command->recycle();
		}
		reorder.clear();
		StateTracker().Reset();
		EndPrepareFrame();
	});
	for (auto&& list : cmdLists) {
		/////////////// Reset
		sets.clear();
		frameRes->ResetScratch();
		updateInfos.clear();
		hostAlloc.Clear();
		/////////////// Preprocess
		for (auto&& cmd : list) {
			cmd->accept(prep);
		}
		/////////////// Bindless read state update
		if (!readArr.empty()) {
			StateTracker().MarkBindlessRead(readArr);
			readArr.clear();
		}
		/////////////// Execute Scratch
		if (!updateInfos.empty()) {
			frameRes->ExecuteScratchAlloc(StateTracker());
		}
		/////////////// Execute batched barrier
		StateTracker().Execute(cmdBuffer);
		/////////////// Update Bindless
		device->UpdateBindless();
		/////////////// Execute copy
		frameRes->ExecuteCopy();
		/////////////// Set argument buffer to read state
		for (auto&& i : sets) {
			StateTracker().MarkBufferRead(i.storageBuffer, BufferReadState::ComputeOrCopy);
		}
		if (!sets.empty())
			StateTracker().Execute(cmdBuffer);
		visitor.buildInfo = updateInfos.data();
		visitor.sets = sets.data();
		/////////////// Postprocess Command
		for (auto&& cmd : list) {
			cmd->accept(visitor);
		}
		if (!accelBuildCmd.empty()) {
			device->vkCmdBuildAccelerationStructuresKHR(
				cmdBuffer->CmdBuffer(),
				accelBuildCmd.size(),
				accelBuildCmd.data(),
				accelRangeCmd.data());
			accelBuildCmd.clear();
			accelRangeCmd.clear();
		}
		/////////////// Compact accel
		if (!compactAccel.empty()) {
			StateTracker().Execute(cmdBuffer);
			Mesh::LoadCompactSize(
				device,
				cmdBuffer->CmdBuffer(),
				&query,
				compactAccel);
			compactAccel.clear();
			ForceSyncInRendering();
			StateTracker().Reset();
			compactSizes.clear();
			compactSizes.resize(compactMesh.size());
			query.Readback(compactSizes);
			for (auto i : vstd::range(compactMesh.size())) {
				compactMesh[i]->Compact(
					cmdBuffer,
					compactSizes[i],
					StateTracker(),
					frameRes);
			}
			compactMesh.clear();
		}
	}
}
LCStream::LCStream(Device const* device)
	: RenderPipeline(device),
	  query(device, 4096),
	  hostAlloc(16384, &hostAllocVisitor),
	  reorder([](uint64_t arr, uint64_t handle) {
		  return reinterpret_cast<BindlessArray*>(arr)->IsPtrInRes(
			  handle);
	  }) {}
LCStream::~LCStream() {}
}// namespace toolhub::vk