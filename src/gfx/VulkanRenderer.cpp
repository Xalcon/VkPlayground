#include "VulkanRenderer.h"
#include <cassert>
#include <set>
#include <vulkan/vulkan.hpp>
#include <SDL_vulkan.h>
#include <spdlog/spdlog.h>
#include "../utils/Rating.hpp"

std::vector<const char*> getExtensions(SDL_Window* window)
{
	uint32_t extCount = 0;
	if (!SDL_Vulkan_GetInstanceExtensions(window, &extCount, nullptr))
		throw std::exception(SDL_GetError());

	std::vector<const char*> extensions;
	extensions.resize(extCount);

	if (!SDL_Vulkan_GetInstanceExtensions(window, &extCount, extensions.data()))
		throw std::exception(SDL_GetError());

#if _DEBUG
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

	return extensions;
}

std::vector<const char*> getLayers()
{
	std::vector<const char*> layers =
	{
#if _DEBUG
		"VK_LAYER_LUNARG_standard_validation"
#endif
	};
	return layers;
}

static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		spdlog::get("vk")->error(pCallbackData->pMessage);
	else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		spdlog::get("vk")->warn(pCallbackData->pMessage);
	else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		spdlog::get("vk")->info(pCallbackData->pMessage);
	else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
		spdlog::get("vk")->trace(pCallbackData->pMessage);
	return true;
}

VulkanRenderer::VulkanRenderer() = default;

VulkanRenderer::~VulkanRenderer()
{
	if (this->device)
	{
		this->device.destroy();
		this->device = nullptr;
	}

	if (this->surface)
	{
		this->instance.destroySurfaceKHR(this->surface);
		this->surface = nullptr;
	}

	this->DestroyDebugCallback();

	if (this->instance)
	{
		this->instance.destroy();
		this->instance = nullptr;
	}
}


void VulkanRenderer::RegisterDebugCallback()
{
	/*vk::DebugUtilsMessengerCreateInfoEXT createInfo(
		vk::DebugUtilsMessengerCreateFlagsEXT(),
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
		debugCallback,
		nullptr);*/
	// this->instance.createDebugUtilsMessengerEXT(createInfo);

	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr;

	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(static_cast<VkInstance>(this->instance), "vkCreateDebugUtilsMessengerEXT");
	func(static_cast<VkInstance>(this->instance), &createInfo, nullptr, &this->callback);
}

void VulkanRenderer::DestroyDebugCallback()
{
	if (this->callback)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(static_cast<VkInstance>(this->instance), "vkDestroyDebugUtilsMessengerEXT");
		func(static_cast<VkInstance>(this->instance), this->callback, nullptr);
		this->callback = 0;
	}
}

void VulkanRenderer::CreateInstance(SDL_Window* window)
{
	auto log = spdlog::get("logger");
	log->info("Initializing vulkan instance");

	vk::ApplicationInfo appInfo("VkPlayground", VK_MAKE_VERSION(0, 1, 0), "unnamed", VK_MAKE_VERSION(0, 1, 0), VK_VERSION_1_1);

	auto extensions = getExtensions(window);
	auto layers = getLayers();

	for (auto& extension : extensions)
		log->info("Requesting extension {0}", extension);

	this->instance = vk::createInstance(vk::InstanceCreateInfo(vk::InstanceCreateFlags(), &appInfo, static_cast<uint32_t>(layers.size()), layers.data(), static_cast<uint32_t>(extensions.size()), extensions.data()));
	assert(instance);

	if (std::find(extensions.begin(), extensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != extensions.end())
		this->RegisterDebugCallback();
}

void VulkanRenderer::CreateSurface(SDL_Window * window)
{
	if (!SDL_Vulkan_CreateSurface(window, this->instance, reinterpret_cast<VkSurfaceKHR*>(&this->surface)))
		throw std::exception(SDL_GetError());
}

float ratePhysicalDevice(vk::PhysicalDevice physicalDevice)
{
	vk::PhysicalDeviceProperties props = physicalDevice.getProperties();
	if (props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
		return 100;
	if (props.deviceType == vk::PhysicalDeviceType::eIntegratedGpu)
		return 50;
	if (props.deviceType == vk::PhysicalDeviceType::eCpu)
		return 10;
	if (props.deviceType == vk::PhysicalDeviceType::eVirtualGpu)
		return 1;

	return -1;
}

void VulkanRenderer::PickPhysicalDevice()
{
	struct Rating
	{
		vk::PhysicalDevice device;
		int32_t score;
	};

	std::vector<Rating> ratings;

	auto log = spdlog::get("logger");

	int i = 0;
	auto deviceList = this->instance.enumeratePhysicalDevices();
	for (auto& device : deviceList)
	{
		auto props = device.getProperties();
		log->info("[{0}] Name: {1}, Driver: {2}, Api: {3}", i++, props.deviceName, props.driverVersion, props.apiVersion);
	}

	auto r = GetBestRatedElement<vk::PhysicalDevice>(deviceList, ratePhysicalDevice);
	if (r.score <= 0)
		throw std::exception("No suitable physical device found");

	this->physicalDevice = r.element;
}

void VulkanRenderer::CreateDevice()
{
	auto families = this->physicalDevice.getQueueFamilyProperties();
	auto graphicsQueue = GetBestRatedElement<vk::QueueFamilyProperties>(families, [](vk::QueueFamilyProperties p)
	{
		float score = -100;
		if(p.queueFlags & vk::QueueFlagBits::eGraphics)
			score += 200.0f;
		if (p.queueFlags & vk::QueueFlagBits::eTransfer)
			score += 10.0f;
		return score;
	});
	
	auto physicalDevice = this->physicalDevice;
	auto surface = this->surface;

	auto presentQueue = GetBestRatedElement<vk::QueueFamilyProperties>(families, [physicalDevice, surface](vk::QueueFamilyProperties p, int index)
	{
		float score = -100;
		if (physicalDevice.getSurfaceSupportKHR(index, surface))
			score += 200;
		return score;
	});

	if (graphicsQueue.score <= 0)
		throw std::exception("Unable to find device queue with graphics support");

	if (presentQueue.score <= 0)
		throw std::exception("Unable to find device queue with present support");
	
	std::set<int> queues;
	queues.emplace(graphicsQueue.index);
	queues.emplace(presentQueue.index);

	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	for (auto& queue : queues)
	{
		float queuePriority = 1.f;
		queueCreateInfos.emplace_back(vk::DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), queue, 1, &queuePriority));
	}
	int size = queueCreateInfos.size();
	vk::DeviceCreateInfo createInfo(vk::DeviceCreateFlags(), size, queueCreateInfos.data());
	this->device = this->physicalDevice.createDevice(createInfo);

	this->queueInfo.graphicsQueue = this->device.getQueue(graphicsQueue.index, 0);
	this->queueInfo.presentQueue = this->device.getQueue(presentQueue.index, 0);
}

void VulkanRenderer::Initialize(SDL_Window* window)
{
	this->CreateInstance(window);
	this->CreateSurface(window);
	this->PickPhysicalDevice();
	this->CreateDevice();
}
