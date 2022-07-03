#include <gpu_collection/bindless_array.h>
#include <vk_runtime/res_state_tracker.h>
#include <vk_runtime/frame_resource.h>
namespace toolhub::vk {
BindlessArray::BindlessArray(
	Device const* device,
	uint arrSize)
	: GPUCollection(device),
	  instanceBuffer(device, arrSize * sizeof(Instance), false, RWState::None),
	  instances(arrSize) {
}
BindlessArray::~BindlessArray() {
	for (auto&& i : instances) {
		if (i.first.index != std::numeric_limits<uint>::max()) {
			device->DeAllocateBindlessIdx(i.first.index);
		}
	}
}
template<typename T>
requires(std::is_base_of_v<GPUCollection, T>) void BindlessArray::AddRef(T const* v, uint& refCount) {
	++refCount;
	refMap.Emplace(reinterpret_cast<uint64>(v), 0).Value()++;
}
template<typename T>
requires(std::is_base_of_v<GPUCollection, T>) void BindlessArray::RemoveRef(T const* v, uint& refCount) {
	if (!v) return;
	--refCount;
	auto ite = refMap.Find(reinterpret_cast<uint64>(v));
	if (!ite) return;
	if (--ite.Value() == 0) {
		refMap.Remove(ite);
	}
}
bool BindlessArray::IsPtrInRes(uint64 handle) const {
	return refMap.Find(handle);
}
void BindlessArray::Bind(uint index, Buffer const* buffer, uint64 offset) {
	auto&& v = updateList.Emplace(index).Value();
	auto& ref = instances[index].second;
	RemoveRef(ref.buffer, ref.refCount);
	AddRef(buffer, ref.refCount);
	ref.buffer = buffer;
	v.flag = (UpdateFlag)((vbyte)v.flag | (vbyte)UpdateFlag::BUFFER);
	v.bufferOffset = offset;
}
void BindlessArray::Bind(uint index, Texture const* tex, vbyte offset) {
	auto&& v = updateList.Emplace(index).Value();
	auto& ref = instances[index].second;
	if (tex->Dimension() == VK_IMAGE_TYPE_2D) {
		RemoveRef(ref.tex2D, ref.refCount);
		AddRef(tex, ref.refCount);
		ref.tex2D = tex;
		v.flag = (UpdateFlag)((vbyte)v.flag | (vbyte)UpdateFlag::TEX2D);
		v.tex2DOffset = offset;
	} else {
		RemoveRef(ref.tex3D, ref.refCount);
		AddRef(tex, ref.refCount);
		ref.tex3D = tex;
		v.flag = (UpdateFlag)((vbyte)v.flag | (vbyte)UpdateFlag::TEX3D);
		v.tex3DOffset = offset;
	}
}
void BindlessArray::UnBindBuffer(uint index) {
	auto&& v = updateList.Emplace(index).Value();
	auto& ref = instances[index].second;
	RemoveRef(ref.buffer, ref.refCount);
	ref.buffer = nullptr;
	v.flag = (UpdateFlag)((vbyte)v.flag | (vbyte)UpdateFlag::BUFFER);
}
void BindlessArray::UnBindTex2D(uint index) {
	auto&& v = updateList.Emplace(index).Value();
	auto& ref = instances[index].second;
	RemoveRef(ref.tex2D, ref.refCount);
	ref.tex2D = nullptr;
	v.flag = (UpdateFlag)((vbyte)v.flag | (vbyte)UpdateFlag::TEX2D);
}
void BindlessArray::UnBindTex3D(uint index) {
	auto&& v = updateList.Emplace(index).Value();
	auto& ref = instances[index].second;
	RemoveRef(ref.tex3D, ref.refCount);
	ref.tex3D = nullptr;
	v.flag = (UpdateFlag)((vbyte)v.flag | (vbyte)UpdateFlag::TEX3D);
}
//rendering
void BindlessArray::Preprocess(
	FrameResource* res,
	ResStateTracker& stateTracker,
	CommandBuffer* cb) {
	stateTracker.MarkBufferWrite(
		&instanceBuffer,
		BufferWriteState::Copy);
	auto ite = updateList.begin();
	BufferView upload = res->AllocateUpload(
		updateList.size() * sizeof(Instance));
	auto iterateFunc = [&] -> vstd::optional<VkBufferCopy> {
		if (ite == updateList.end())
			return {};
		vstd::optional<VkBufferCopy> opt;
		opt.New();
		auto index = ite->first;
		auto offset = size_t(index) * sizeof(Instance);
		auto uploadOffset = upload.offset + offset;
		auto& inst = instances[index];
		auto& refOp = ite->second;
		auto& ref = inst.second;
		opt->srcOffset = uploadOffset;
		opt->dstOffset = offset;
		opt->size = sizeof(Instance);
		if (ref.refCount == 0) {
			if (inst.first.index != std::numeric_limits<uint>::max()) {
				res->AddDisposeBindlessIdx(inst.first.index);
				inst.first.index = std::numeric_limits<uint>::max();
			}
		} else {
			if (inst.first.index == std::numeric_limits<uint>::max()) {
				inst.first.index = device->AllocateBindlessIdx();
			}
		}
		// update buffer
		if ((static_cast<vbyte>(refOp.flag) & static_cast<vbyte>(UpdateFlag::BUFFER)) != 0) {
			if (ref.buffer) {
				device->AddBindlessBufferUpdateCmd(
					inst.first.index,
					BufferView(ref.buffer, refOp.bufferOffset));
			}
			inst.second.buffer = ref.buffer;
		}
		// update tex2d
		if ((static_cast<vbyte>(refOp.flag) & static_cast<vbyte>(UpdateFlag::TEX2D)) != 0) {
			if (ref.tex2D) {
				device->AddBindlessTex2DUpdateCmd(
					inst.first.index,
					TexView(ref.tex2D, refOp.tex2DOffset));
			}
			inst.second.tex2D = ref.tex2D;
		}
		// update tex3d
		if ((static_cast<vbyte>(refOp.flag) & static_cast<vbyte>(UpdateFlag::TEX3D)) != 0) {
			if (ref.tex3D) {
				device->AddBindlessTex3DUpdateCmd(
					inst.first.index,
					TexView(ref.tex3D, refOp.tex3DOffset));
			}
			inst.second.tex3D = ref.tex3D;
		}
		upload.buffer->CopyValueFrom(inst.first, uploadOffset);
		++ite;
		return opt;
	};
	res->AddCopyCmd(
		upload.buffer,
		&instanceBuffer,
		iterateFunc,
		updateList.size());
	updateList.Clear();
}

}// namespace toolhub::vk