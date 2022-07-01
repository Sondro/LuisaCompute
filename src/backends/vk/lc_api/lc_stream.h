#pragma once
#include <runtime/device.h>
#include <runtime/command.h>
#include <runtime/command_buffer.h>
#include <vk_runtime/render_pipeline.h>
#include <vk_rtx/mesh.h>
#include <vk_rtx/accel.h>
#include <runtime/command_reorder_visitor.h>
#include <vk_rtx/query.h>
using namespace luisa;
using namespace luisa::compute;
namespace toolhub::vk {
struct UniformBuffer {
	VkDescriptorSet set;
	BufferView storageBuffer;
	UniformBuffer(
		VkDescriptorSet set,
		BufferView storageBuffer) : set(set), storageBuffer(storageBuffer) {}
};
class LCStream : public RenderPipeline {
public:
	vstd::vector<BuildInfo> updateInfos;
	vstd::vector<BindlessArray*> readArr;
	vstd::vector<BindResource> bindVec;
	vstd::vector<UniformBuffer> sets;
	vstd::vector<Mesh*> compactMesh;
	vstd::vector<size_t> compactSizes;
	vstd::vector<VkAccelerationStructureKHR> compactAccel;
	CommandReorderVisitor reorder;
	Query query;

	void DispatchCmd(CommandList&& cmdList);
	LCStream(Device const* device);
	~LCStream();
};
}// namespace toolhub::vk