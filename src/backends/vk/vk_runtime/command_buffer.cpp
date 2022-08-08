#include "command_pool.h"
#include "command_buffer.h"
#include "frame_resource.h"
#include <gpu_collection/buffer.h>
#include <shader/descriptorset_manager.h>
#include <shader/compute_shader.h>
#include <shader/rt_shader.h>
#include <vk_runtime/res_state_tracker.h>
#include <vk_rtx/accel.h>
#include <shader/pipeline_layout.h>
namespace toolhub::vk {
CommandBuffer::~CommandBuffer() {
	if (cmdBuffer) {
		ThrowIfFailed(vkEndCommandBuffer(cmdBuffer));
	}
}
CommandBuffer::CommandBuffer(
	DescriptorSetManager* descManager,
	Device const* device,
	VkCommandBuffer cmdBuffer,
	FrameResource* pool)
	: Resource(device),
	  descManager(descManager),
	  pool(pool), cmdBuffer(cmdBuffer) {
	VkCommandBufferBeginInfo info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	ThrowIfFailed(vkResetCommandBuffer(cmdBuffer, 0));
	ThrowIfFailed(vkBeginCommandBuffer(cmdBuffer, &info));
}
void CommandBuffer::PreprocessCopyBuffer(
	ResStateTracker& stateTracker,
	Buffer const* srcBuffer,
	size_t srcOffset,
	Buffer const* dstBuffer,
	size_t dstOffset,
	size_t size) {
	stateTracker.MarkBufferRead(
		BufferView(srcBuffer, srcOffset, size),
		BufferReadState::ComputeOrCopy);
	stateTracker.MarkBufferWrite(
		BufferView(dstBuffer, dstOffset, size),
		BufferWriteState::Copy);
}
void CommandBuffer::CopyBuffer(
	Buffer const* srcBuffer,
	size_t srcOffset,
	Buffer const* dstBuffer,
	size_t dstOffset,
	size_t size) {
	VkBufferCopy copyRegion{srcOffset, dstOffset, size};

	vkCmdCopyBuffer(cmdBuffer, srcBuffer->GetResource(), dstBuffer->GetResource(), 1, &copyRegion);
}
VkDescriptorSet CommandBuffer::PreprocessDispatch(
	PipelineLayout const& layout,
	vstd::span<BindResource const> binds) {
	return descManager->Allocate(
		layout.DescSetLayout(),
		layout.PropertiesTypes(),
		binds);
}
void CommandBuffer::Dispatch(
	VkDescriptorSet set,
	ComputeShader const* cs,
	uint3 dispatchCount) {
	vkCmdBindPipeline(
		cmdBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		cs->pipeline);

	VkDescriptorSet sets[] = {
		set,
		device->bindlessBufferSet,
		device->bindlessTex2DSet,
		device->bindlessTex3DSet,
		device->samplerSet};
	vkCmdBindDescriptorSets(
		cmdBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		cs->Layout().Layout(),
		0, vstd::array_count(sets), sets,
		0, nullptr);
	vkCmdDispatch(
		cmdBuffer,
		(dispatchCount.x + (cs->threadGroupSize.x - 1)) / cs->threadGroupSize.x,
		(dispatchCount.y + (cs->threadGroupSize.y - 1)) / cs->threadGroupSize.y,
		(dispatchCount.z + (cs->threadGroupSize.z - 1)) / cs->threadGroupSize.z);
}
void CommandBuffer::Dispatch(
	VkDescriptorSet set,
	RTShader const* cs,
	uint3 dispatchCount) {
	vkCmdBindPipeline(
		cmdBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		cs->pipeline);

	VkDescriptorSet sets[] = {
		set,
		device->bindlessBufferSet,
		device->bindlessTex2DSet,
		device->bindlessTex3DSet,
		device->samplerSet};
	vkCmdBindDescriptorSets(
		cmdBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		cs->Layout().Layout(),
		0, vstd::array_count(sets), sets,
		0, nullptr);
	device->vkCmdTraceRaysKHR(
		cmdBuffer,
		vstd::get_rvalue_ptr(cs->GetSBTAddress(VK_SHADER_STAGE_RAYGEN_BIT_KHR)),
		vstd::get_rvalue_ptr(cs->GetSBTAddress(VK_SHADER_STAGE_MISS_BIT_KHR)),
		vstd::get_rvalue_ptr(cs->GetSBTAddress(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)),
		vstd::get_rvalue_ptr(cs->GetSBTAddress(VK_SHADER_STAGE_CALLABLE_BIT_KHR)),
		dispatchCount.x, dispatchCount.y, dispatchCount.z);
}
}// namespace toolhub::vk