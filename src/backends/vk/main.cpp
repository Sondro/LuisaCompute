#include <components/device.h>
#include <vstl/BinaryReader.h>
#include <shader/compute_shader.h>
#include <shader/shader_code.h>
#include <gpu_collection/buffer.h>
#include <shader/descriptor_pool.h>
#include <shader/descriptorset_manager.h>
#include <vk_runtime/command_pool.h>
#include <vk_runtime/frame_resource.h>
#include <vk_runtime/res_state_tracker.h>
#include <gpu_collection/texture.h>
#include <vk_rtx/mesh.h>
#include <vk_rtx/accel.h>
#include <vk_rtx/query.h>
#include <dxc/dxc_util.h>
using namespace toolhub::vk;
namespace toolhub::vk {
// void TestBuffer(Device const* device, vstd::span<vbyte const> block);
// void TestBindless(Device const* device, vstd::span<vbyte const> block);
// void TestRayQuery(Device const* device, vstd::span<vbyte const> block);
// void CompileAndTest(
// 	Device const* device,
// 	vstd::string const& shaderName,
// 	vstd::move_only_func<void(Device const* device, vstd::span<vbyte const> block)> const& func) {
// 	vstd::string shaderCode;
// 	{
// 		BinaryReader reader(shaderName);
// 		shaderCode.resize(reader.GetLength());
// 		reader.Read(shaderCode.data(), shaderCode.size());
// 	}
// 	toolhub::directx::DXShaderCompiler compiler;
// 	auto compResult = compiler.CompileCompute(
// 		shaderCode,
// 		true);
// 	auto errMsg = compResult.try_get<vstd::string>();
// 	if (errMsg) {
// 		std::cout << "Compile error: " << *errMsg << '\n';
// 		return;
// 	}
// 	auto&& byteCode = compResult.get<0>();
// 	vstd::span<vbyte const> resultArr(
// 		byteCode->GetBufferPtr(),
// 		byteCode->GetBufferSize());
// 	func(device, resultArr);
// }
}// namespace toolhub::vk
// int main() {
// 	auto instance = toolhub::vk::Device::InitVkInstance();
// 	auto device = toolhub::vk::Device::CreateDevice(instance, nullptr, 0);
// 	//CompileAndTest(device, "test_rayquery.compute", TestRayQuery);
// 	std::cout << "max uniform range: " << device->limits.maxUniformBufferRange << '\n';

// 	delete device;
// 	vkDestroyInstance(instance, toolhub::vk::Device::Allocator());
// 	std::cout << "finished\n";
// 	return 0;
// }