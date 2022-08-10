#include "device.h"
#include <vstl/small_vector.h>
#include <gpu_allocator/gpu_allocator.h>
#include <shader/descriptorset_manager.h>
#include <vk_types/buffer_view.h>
#include <vk_types/tex_view.h>
#include <gpu_collection/buffer.h>
#include <gpu_collection/texture.h>
#include <core/dynamic_module.h>
namespace toolhub::vk {
namespace detail {
static constexpr bool USE_VALIDATION = true;
static auto validationLayers = {"VK_LAYER_KHRONOS_validation"};
}// namespace detail
VkInstance Device::InitVkInstance() {
	VkInstance instance;
#ifndef NDEBUG
	auto Check = [&] {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		vstd::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
		if constexpr (detail::USE_VALIDATION) {
			for (const char* layerName : detail::validationLayers) {
				bool layerFound = false;

				for (const auto& layerProperties : availableLayers) {
					if (strcmp(layerName, layerProperties.layerName) == 0) {
						layerFound = true;
						break;
					}
				}

				if (!layerFound) {
					return false;
				}
			}
		}
		return true;
	};
	if (!Check()) {
		VEngine_Log("validation layers requested, but not available!");
		VENGINE_EXIT;
	}
#endif
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = nullptr;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = nullptr;
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VulkanApiVersion;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	vstd::vector<char const*> requiredExts;
	{
/*		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		requiredExts = vstd::span<const char*>{glfwExtensions, glfwExtensionCount};*/
#ifndef NDEBUG
		requiredExts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
	}
	createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExts.size());
	createInfo.ppEnabledExtensionNames = requiredExts.data();
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
#ifndef NDEBUG
	if (detail::USE_VALIDATION) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(detail::validationLayers.size());
		createInfo.ppEnabledLayerNames = detail::validationLayers.begin();
	} else {
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
	}
	auto populateDebugMessengerCreateInfo = [](VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback =
			[](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			   VkDebugUtilsMessageTypeFlagsEXT messageType,
			   const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			   void* pUserData) {
				if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
					// Message is important enough to show
					std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
				}

				return VK_FALSE;
			};
	};
	populateDebugMessengerCreateInfo(debugCreateInfo);
	createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#else
	createInfo.enabledLayerCount = 0;
	createInfo.pNext = nullptr;
#endif
	// Create Instance
	ThrowIfFailed(vkCreateInstance(&createInfo, Device::Allocator(), &instance));
	return instance;
}
namespace detail {
class VulkanAllocatorImpl {
public:
	/*
	vstd::funcPtr_t<void*(size_t)> mallocFunc = nullptr;
	vstd::funcPtr_t<void(void*)> freeFunc = nullptr;
	vstd::funcPtr_t<void*(void*, size_t)> reallocFunc = nullptr;
	std::optional<luisa::DynamicModule> dll;*/
	VkAllocationCallbacks allocator;
	VulkanAllocatorImpl() {
		/*
		dll = luisa::DynamicModule::load("mimalloc");
		mallocFunc = (decltype(mallocFunc))dll->address("mi_malloc");
		freeFunc = (decltype(freeFunc))dll->address("mi_free");
		reallocFunc = (decltype(reallocFunc))dll->address("mi_realloc");*/
		allocator.pUserData = this;
		allocator.pfnAllocation =
			[](void* pUserData,
			   size_t size,
			   size_t alignment,
			   VkSystemAllocationScope allocationScope) {
				return vengine_malloc(size);
			};
		allocator.pfnFree =
			[](void* pUserData,
			   void* pMemory) {
				vengine_free(pMemory);
			};
		allocator.pfnReallocation =
			[](void* pUserData,
			   void* pOriginal,
			   size_t size,
			   size_t alignment,
			   VkSystemAllocationScope allocationScope) {
				return vengine_realloc(pOriginal, size);
			};
		allocator.pfnInternalAllocation =
			[](void* pUserData,
			   size_t size,
			   VkInternalAllocationType allocationType,
			   VkSystemAllocationScope allocationScope) {

			};
		allocator.pfnInternalFree =
			[](void* pUserData,
			   size_t size,
			   VkInternalAllocationType allocationType,
			   VkSystemAllocationScope allocationScope) {};
	}
};
static VulkanAllocatorImpl gVulkanAllocatorImpl;
}// namespace detail
VkAllocationCallbacks* Device::Allocator() {
	return &detail::gVulkanAllocatorImpl.allocator;
}

Device::Device()
	: bindlessStackAlloc(4096, &mallocVisitor) {
}
void Device::Init() {
	psoCache.headerSize = sizeof(VkPipelineCacheHeaderVersionOne);
	psoCache.headerVersion = VK_PIPELINE_CACHE_HEADER_VERSION_ONE;
	psoCache.vendorID = deviceProperties.vendorID;
	psoCache.deviceID = deviceProperties.deviceID;
	memcpy(psoCache.pipelineCacheUUID, deviceProperties.pipelineCacheUUID, VK_UUID_SIZE);
	gpuAllocator = vstd::create_unique(new GPUAllocator(this));
	manager = vstd::create_unique(new DescriptorSetManager(this));
	// create sampler desc-set layout
	VkDescriptorSetLayoutBinding binding =
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT, 0, DescriptorPool::MAX_SAMP);
	VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo({&binding, 1});
	ThrowIfFailed(vkCreateDescriptorSetLayout(device, &descriptorLayout, Device::Allocator(), &samplerSetLayout));
	pool = vstd::create_unique(new DescriptorPool(this));
	samplerSet = pool->Allocate(samplerSetLayout);
	VkFilter filters[4] = {
		VK_FILTER_NEAREST,
		VK_FILTER_LINEAR,
		VK_FILTER_LINEAR,
		VK_FILTER_LINEAR};
	VkSamplerMipmapMode mipFilter[4] = {
		VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_MIPMAP_MODE_LINEAR,
		VK_SAMPLER_MIPMAP_MODE_LINEAR};
	VkSamplerAddressMode addressMode[4] = {
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER};

	size_t idx = 0;
	VkDescriptorImageInfo imageInfos[DescriptorPool::MAX_SAMP];
	for (auto x : vstd::range(4))
		for (auto y : vstd::range(4)) {
			auto d = vstd::create_disposer([&] { ++idx; });
			auto&& samp = samplers[idx];
			VkSamplerCreateInfo createInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
			createInfo.magFilter = filters[y];
			createInfo.minFilter = filters[y];
			createInfo.mipmapMode = mipFilter[y];
			createInfo.addressModeU = addressMode[x];
			createInfo.addressModeV = addressMode[x];
			createInfo.addressModeW = addressMode[x];
			createInfo.mipLodBias = 0;
			createInfo.anisotropyEnable = y == 3;
			createInfo.maxAnisotropy = DescriptorPool::MAX_SAMP;
			createInfo.minLod = 0;
			createInfo.maxLod = 255;
			vkCreateSampler(
				device,
				&createInfo,
				Device::Allocator(),
				&samp);
			imageInfos[idx].sampler = samp;
		}
	VkWriteDescriptorSet writeDesc{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
	writeDesc.dstSet = samplerSet;
	writeDesc.dstBinding = 0;
	writeDesc.dstArrayElement = 0;
	writeDesc.descriptorCount = DescriptorPool::MAX_SAMP;
	writeDesc.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	writeDesc.pImageInfo = imageInfos;
	vkUpdateDescriptorSets(device, 1, &writeDesc, 0, nullptr);
	InitBindless();
	auto SafeCastFuncPtr = []<typename T>(PFN_vkVoidFunction num) {
		assert(num != 0);
		return reinterpret_cast<T>(num);
	};

	vkGetAccelerationStructureBuildSizesKHR = SafeCastFuncPtr.operator()<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
	vkCreateAccelerationStructureKHR = SafeCastFuncPtr.operator()<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
	vkDestroyAccelerationStructureKHR = SafeCastFuncPtr.operator()<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
	vkCmdBuildAccelerationStructuresKHR = SafeCastFuncPtr.operator()<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));
	vkGetAccelerationStructureDeviceAddressKHR = SafeCastFuncPtr.operator()<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR"));
	vkCmdWriteAccelerationStructuresPropertiesKHR = SafeCastFuncPtr.operator()<PFN_vkCmdWriteAccelerationStructuresPropertiesKHR>(vkGetDeviceProcAddr(device, "vkCmdWriteAccelerationStructuresPropertiesKHR"));
	vkCmdCopyAccelerationStructureKHR = SafeCastFuncPtr.operator()<PFN_vkCmdCopyAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkCmdCopyAccelerationStructureKHR"));
	vkCmdTraceRaysKHR = SafeCastFuncPtr.operator()<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR"));
	vkGetRayTracingShaderGroupHandlesKHR = SafeCastFuncPtr.operator()<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR"));
	vkCreateRayTracingPipelinesKHR = SafeCastFuncPtr.operator()<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));
}
void Device::InitBindless() {
	VkDescriptorSetLayoutBinding setLayoutBinding{};
	setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	setLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	setLayoutBinding.binding = 0;
	setLayoutBinding.descriptorCount = DescriptorPool::MAX_BINDLESS_SIZE;
	VkDescriptorSetVariableDescriptorCountAllocateInfo bindlessSize{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO};
	bindlessSize.pDescriptorCounts = &setLayoutBinding.descriptorCount;
	bindlessSize.descriptorSetCount = 1;

	VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo({&setLayoutBinding, 1});
	descriptorLayout.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
	VkDescriptorSetLayoutBindingFlagsCreateInfoEXT setLayoutBindingFlags{};
	setLayoutBindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
	setLayoutBindingFlags.bindingCount = 1;
	VkDescriptorBindingFlagsEXT descriptorBindingFlags =
		VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
	setLayoutBindingFlags.pBindingFlags = &descriptorBindingFlags;
	descriptorLayout.pNext = &setLayoutBindingFlags;
	ThrowIfFailed(vkCreateDescriptorSetLayout(device, &descriptorLayout, Device::Allocator(), &bindlessTex2DSetLayout));
	bindlessTex2DSet = pool->Allocate(bindlessTex2DSetLayout, &bindlessSize);
	ThrowIfFailed(vkCreateDescriptorSetLayout(device, &descriptorLayout, Device::Allocator(), &bindlessTex3DSetLayout));
	bindlessTex3DSet = pool->Allocate(bindlessTex3DSetLayout, &bindlessSize);
	setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	ThrowIfFailed(vkCreateDescriptorSetLayout(device, &descriptorLayout, Device::Allocator(), &bindlessBufferSetLayout));
	bindlessBufferSet = pool->Allocate(bindlessBufferSetLayout, &bindlessSize);
	bindlessIdx.push_back_func(DescriptorPool::MAX_BINDLESS_SIZE, [](size_t i) { return i; });
}
uint Device::AllocateBindlessIdx() const {
	std::lock_guard lck(allocIdxMtx);
	return bindlessIdx.erase_last();
}
void Device::DeAllocateBindlessIdx(uint index) const {
	std::lock_guard lck(allocIdxMtx);
	bindlessIdx.emplace_back(index);
}
Device::~Device() {
	pool->Destroy(samplerSet);
	pool->Destroy(bindlessTex2DSet);
	pool->Destroy(bindlessTex3DSet);
	pool->Destroy(bindlessBufferSet);
	vkDestroyDescriptorSetLayout(
		device,
		bindlessTex2DSetLayout,
		Device::Allocator());
	vkDestroyDescriptorSetLayout(
		device,
		bindlessTex3DSetLayout,
		Device::Allocator());
	vkDestroyDescriptorSetLayout(
		device,
		bindlessBufferSetLayout,
		Device::Allocator());
	vkDestroyDescriptorSetLayout(
		device,
		samplerSetLayout,
		Device::Allocator());
	for (auto&& i : samplers) {
		vkDestroySampler(device, i, Device::Allocator());
	}

	gpuAllocator = nullptr;
	pool = nullptr;
	vkDestroyDevice(device, Device::Allocator());
}

//TODO
Device* Device::CreateDevice(
	VkInstance instance,
	VkSurfaceKHR surface,
	uint physicalDeviceIndex,
	void* placedMemory) {
	std::initializer_list<char const*> requiredFeatures = {
		"VK_EXT_descriptor_indexing",
		"VK_EXT_host_query_reset",
		"VK_KHR_buffer_device_address",
		"VK_KHR_deferred_host_operations",
		"VK_KHR_acceleration_structure",
		"VK_KHR_variable_pointers",
		"VK_KHR_ray_tracing_pipeline"};
	auto checkDeviceExtensionSupport = [&](VkPhysicalDevice device) {
		uint32_t extensionCount;
		ThrowIfFailed(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr));
		vstd::vector<VkExtensionProperties> availableExtensions(extensionCount);
		ThrowIfFailed(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data()));
		vstd::HashMap<vstd::string_view> requiredExtensions(requiredFeatures.size());
		for (auto&& i : requiredFeatures) {
			requiredExtensions.Emplace(i);
		}
		size_t supportedExt = 0;
		for (const auto& extension : availableExtensions) {
			char const* ptr = extension.extensionName;
			if (requiredExtensions.Find(ptr)) {
				supportedExt++;
			}
		}
		return supportedExt == requiredExtensions.Size();
	};
	VkPhysicalDeviceProperties2 deviceProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
	VkPhysicalDeviceFeatures2 deviceFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
	deviceProperties.pNext = &rayTracingProperties;

	auto isDeviceSuitable = [&](VkPhysicalDevice device) {
		vkGetPhysicalDeviceProperties2(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures2(device, &deviceFeatures);

		if (deviceProperties.properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			return false;
		bool extensionsSupported = checkDeviceExtensionSupport(device);
		//TODO: probably need other checks, like swapchain, etc
		return extensionsSupported;
	};
	uint deviceCount = 0;
	ThrowIfFailed(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
	if (deviceCount == 0) {
		return nullptr;
	}

	vstd::small_vector<VkPhysicalDevice> devices(deviceCount);
	ThrowIfFailed(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));
	auto physicalDevice = devices[std::min<uint>(physicalDeviceIndex, deviceCount - 1)];
	if (!isDeviceSuitable(physicalDevice))
		return nullptr;
	vstd::optional<uint32_t> computeFamily;
	vstd::optional<uint32_t> presentFamily;

	auto findQueueFamilies = [&]() {
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

		vstd::small_vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());
		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (!computeFamily && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
				computeFamily = i;
			}
			if (surface) {
				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
				if (presentSupport) {
					presentFamily = i;
				}
			}
			++i;
		}
	};
	findQueueFamilies();
	vstd::small_vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	vstd::optional<uint> uniqueQueueFamilies[] = {
		computeFamily,
		presentFamily};
	float queuePriority = 1.0f;
	for (auto&& queueFamily : uniqueQueueFamilies) {
		if (!queueFamily) continue;
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = *queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}
	//check bindless

	/*VkPhysicalDeviceDescriptorIndexingFeaturesEXT indexingFeatures{};
	indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
	indexingFeatures.pNext = nullptr;
	VkPhysicalDeviceFeatures2 deviceFeatures{};
	deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures.pNext = &indexingFeatures;
	vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures);
	//if not support bindless, return
	if (!indexingFeatures.runtimeDescriptorArray
		|| !indexingFeatures.descriptorBindingVariableDescriptorCount
		|| !indexingFeatures.shaderSampledImageArrayNonUniformIndexing) {
		return nullptr;
	}*/

	//bindless
	VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT};
	indexingFeatures.runtimeDescriptorArray = VK_TRUE;
	indexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
	indexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	indexingFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
	indexingFeatures.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
	indexingFeatures.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
	indexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
	VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddresFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES};
	enabledBufferDeviceAddresFeatures.bufferDeviceAddress = VK_TRUE;
	enabledBufferDeviceAddresFeatures.pNext = &indexingFeatures;

	VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingPipelineFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
	enabledRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
	enabledRayTracingPipelineFeatures.pNext = &enabledBufferDeviceAddresFeatures;

	VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
	enabledAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
	enabledAccelerationStructureFeatures.pNext = &enabledRayTracingPipelineFeatures;

	/*	VkPhysicalDeviceRayQueryFeaturesKHR enabledRayQueryFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};
	enabledRayQueryFeatures.rayQuery = VK_TRUE;
	enabledRayQueryFeatures.pNext = &enabledAccelerationStructureFeatures;
*/
	VkPhysicalDeviceVariablePointersFeatures variablePointerFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTERS_FEATURES};
	variablePointerFeatures.variablePointers = VK_TRUE;
	variablePointerFeatures.variablePointersStorageBuffer = VK_TRUE;
	variablePointerFeatures.pNext = &enabledAccelerationStructureFeatures;
	VkPhysicalDeviceHostQueryResetFeatures enableQueryReset{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES};
	enableQueryReset.hostQueryReset = VK_TRUE;
	enableQueryReset.pNext = &variablePointerFeatures;

	auto featureLinkQueue = &enableQueryReset;

	VkDeviceCreateInfo createInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
	createInfo.pNext = featureLinkQueue;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures.features;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredFeatures.size());
	createInfo.ppEnabledExtensionNames = requiredFeatures.begin();
	if (detail::USE_VALIDATION) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(detail::validationLayers.size());
		createInfo.ppEnabledLayerNames = detail::validationLayers.begin();
	} else {
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
	}
	VkDevice device;
	auto createDeviceResult = vkCreateDevice(physicalDevice, &createInfo, Device::Allocator(), &device);
	if (createDeviceResult != VK_SUCCESS) {
		return nullptr;
	}
	if (!computeFamily)
		return nullptr;
	Device* result = placedMemory ? new (placedMemory) Device() : new Device();
	if (computeFamily) {
		vkGetDeviceQueue(device, *computeFamily, 0, &result->computeQueue);
		result->computeFamily = *computeFamily;
	}
	if (presentFamily) {
		vkGetDeviceQueue(device, *presentFamily, 0, &result->presentQueue);
		result->presentFamily = presentFamily;
	} else {
		result->presentQueue = nullptr;
	}
	result->physicalDevice = physicalDevice;
	result->device = device;
	result->limits = deviceProperties.properties.limits;
	result->instance = instance;
	result->rayTracingProperties = rayTracingProperties;
	result->deviceProperties = deviceProperties.properties;
	result->Init();
	return result;
}

void Device::AddBindlessBufferUpdateCmd(size_t index, BufferView const& buffer) const {
	std::lock_guard lck(updateBindlessMtx);
	auto&& writeDesc = bindlessWriteRes.emplace_back();
	memset(&writeDesc, 0, sizeof(VkWriteDescriptorSet));
	writeDesc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDesc.dstSet = bindlessBufferSet;
	writeDesc.dstBinding = 0;
	writeDesc.dstArrayElement = index;
	writeDesc.descriptorCount = 1;
	writeDesc.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	auto bf = bindlessStackAlloc.Allocate(sizeof(VkDescriptorBufferInfo));
	auto ptr = reinterpret_cast<VkDescriptorBufferInfo*>(bf.handle + bf.offset);
	*ptr = buffer.buffer->GetDescriptor(
		buffer.offset,
		buffer.size);
	writeDesc.pBufferInfo = ptr;
}
void Device::AddBindlessTex2DUpdateCmd(size_t index, TexView const& tex) const {
	std::lock_guard lck(updateBindlessMtx);
	auto&& writeDesc = bindlessWriteRes.emplace_back();
	memset(&writeDesc, 0, sizeof(VkWriteDescriptorSet));
	writeDesc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDesc.dstSet = bindlessTex2DSet;
	writeDesc.dstBinding = 0;
	writeDesc.dstArrayElement = index;
	writeDesc.descriptorCount = 1;
	writeDesc.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	auto bf = bindlessStackAlloc.Allocate(sizeof(VkDescriptorImageInfo));
	auto ptr = reinterpret_cast<VkDescriptorImageInfo*>(bf.handle + bf.offset);
	*ptr = tex.tex->GetDescriptor(
		tex.mipOffset,
		tex.mipCount);
	writeDesc.pImageInfo = ptr;
}
void Device::AddBindlessTex3DUpdateCmd(size_t index, TexView const& tex) const {
	std::lock_guard lck(updateBindlessMtx);
	auto&& writeDesc = bindlessWriteRes.emplace_back();
	memset(&writeDesc, 0, sizeof(VkWriteDescriptorSet));
	writeDesc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDesc.dstSet = bindlessTex3DSet;
	writeDesc.dstBinding = 0;
	writeDesc.dstArrayElement = index;
	writeDesc.descriptorCount = 1;
	writeDesc.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	auto bf = bindlessStackAlloc.Allocate(sizeof(VkDescriptorImageInfo));
	auto ptr = reinterpret_cast<VkDescriptorImageInfo*>(bf.handle + bf.offset);
	*ptr = tex.tex->GetDescriptor(
		tex.mipOffset,
		tex.mipCount);
	writeDesc.pImageInfo = ptr;
}

void Device::UpdateBindless() const {
	std::lock_guard lck(updateBindlessMtx);
	if (bindlessWriteRes.empty()) return;
	vkUpdateDescriptorSets(device, bindlessWriteRes.size(), bindlessWriteRes.data(), 0, nullptr);
	bindlessWriteRes.clear();
	bindlessStackAlloc.Clear();
}
}// namespace toolhub::vk