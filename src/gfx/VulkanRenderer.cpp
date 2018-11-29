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
	const char* loggerName;
	if (type == VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
		loggerName = "vk-perf";
	else if (type == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
		loggerName = "vk-val";
	else
		loggerName = "vk-general";

	if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		spdlog::get(loggerName)->error(pCallbackData->pMessage);
	else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		spdlog::get(loggerName)->warn(pCallbackData->pMessage);
	else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		spdlog::get(loggerName)->info(pCallbackData->pMessage);
	else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
		spdlog::get(loggerName)->trace(pCallbackData->pMessage);

	return VK_FALSE;
}

VulkanRenderer::VulkanRenderer() = default;

VulkanRenderer::~VulkanRenderer()
{
	if(this->swapChain)
	{
		this->device.destroySwapchainKHR(this->swapChain);
		this->swapChain = nullptr;
		this->swapChainImages.clear();
	}

	if (this->surface)
	{
		this->instance.destroySurfaceKHR(this->surface);
		this->surface = nullptr;
	}

	if (this->device)
	{
		this->device.destroy();
		this->device = nullptr;
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

	const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
		static_cast<VkInstance>(this->instance), "vkCreateDebugUtilsMessengerEXT"));
	func(static_cast<VkInstance>(this->instance), &createInfo, nullptr, &this->callback);
}

void VulkanRenderer::DestroyDebugCallback()
{
	if (this->callback)
	{
		const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
			static_cast<VkInstance>(this->instance), "vkDestroyDebugUtilsMessengerEXT"));
		func(static_cast<VkInstance>(this->instance), this->callback, nullptr);
		this->callback = 0;
	}
}

void VulkanRenderer::CreateInstance(SDL_Window* window)
{
	auto log = spdlog::get("logger");
	log->info("Initializing vulkan instance");

	vk::ApplicationInfo appInfo("VkPlayground", VK_MAKE_VERSION(0, 1, 0), "unnamed", VK_MAKE_VERSION(0, 1, 0), VK_API_VERSION_1_1);

	auto extensions = getExtensions(window);
	auto layers = getLayers();

	for (auto& extension : extensions)
		log->info("Requesting extension {0}", extension);

	this->instance = vk::createInstance(vk::InstanceCreateInfo({}, &appInfo, static_cast<uint32_t>(layers.size()), layers.data(), static_cast<uint32_t>(extensions.size()), extensions.data()));
	assert(instance);

	if (std::find(extensions.begin(), extensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != extensions.end())
		this->RegisterDebugCallback();
}

void VulkanRenderer::CreateSurface(SDL_Window* window)
{
	if (!SDL_Vulkan_CreateSurface(window, static_cast<VkInstance>(this->instance), reinterpret_cast<VkSurfaceKHR*>(&this->surface)))
		throw std::exception(SDL_GetError());
}

float ratePhysicalDevice(const vk::PhysicalDevice& physicalDevice)
{
	auto extensions = physicalDevice.enumerateDeviceExtensionProperties();
	if (std::find_if(extensions.begin(), extensions.end(), [](vk::ExtensionProperties& ext) { return strcmp(ext.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME); }) == extensions.end())
		return -1;

	const auto props = physicalDevice.getProperties();
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
	auto log = spdlog::get("logger");

	auto i = 0;
	auto deviceList = this->instance.enumeratePhysicalDevices();
	for (auto& device : deviceList)
	{
		auto props = device.getProperties();
		log->info("[{0}] Name: {1}, Driver: {2}, Api: {3}", i++, props.deviceName, props.driverVersion, props.apiVersion);
	}

	const auto r = GetBestRatedElement<vk::PhysicalDevice>(deviceList, ratePhysicalDevice);
	if (r.score <= 0)
		throw std::exception("No suitable physical device found");

	this->physicalDevice = r.element;
}

void VulkanRenderer::CreateDevice()
{
	const auto families = this->physicalDevice.getQueueFamilyProperties();
	auto graphicsQueue = GetBestRatedElement<vk::QueueFamilyProperties>(families, [](const auto& p)
	{
		float score = -100;
		if (p.queueFlags & vk::QueueFlagBits::eGraphics)
			score += 200.0f;
		if (p.queueFlags & vk::QueueFlagBits::eTransfer)
			score += 10.0f;
		return score;
	});

	auto physicalDevice = this->physicalDevice;
	auto surface = this->surface;

	auto presentQueue = GetBestRatedElement<vk::QueueFamilyProperties>(families, [physicalDevice, surface](const auto& p, int index)
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
		auto queuePriority = 1.f;
		queueCreateInfos.emplace_back(vk::DeviceQueueCreateInfo({}, queue, 1, &queuePriority));
	}

	std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	const vk::DeviceCreateInfo createInfo({}, static_cast<uint32_t>(queueCreateInfos.size()), queueCreateInfos.data(), 0, nullptr, static_cast<uint32_t>(deviceExtensions.size()), deviceExtensions.data());
	this->device = this->physicalDevice.createDevice(createInfo);

	this->queueInfo.graphicsQueueFamilyIndex = graphicsQueue.index;
	this->queueInfo.presentQueueFamilyIndex = presentQueue.index;
	this->queueInfo.graphicsQueue = this->device.getQueue(graphicsQueue.index, 0);
	this->queueInfo.presentQueue = this->device.getQueue(presentQueue.index, 0);
}

void VulkanRenderer::CreateSwapChain(SDL_Window* window)
{
	SwapChainSupportDetails details;
	details.capabilities = this->physicalDevice.getSurfaceCapabilitiesKHR(this->surface);
	details.formats = this->physicalDevice.getSurfaceFormatsKHR(this->surface);
	details.presentModes = this->physicalDevice.getSurfacePresentModesKHR(this->surface);

	if (details.formats.empty())
		throw std::exception("SwapChain incompatible: no supported surface formats found");

	if (details.presentModes.empty())
		throw std::exception("SwapChain incompatible: no supported present modes found");

	vk::SurfaceFormatKHR format;

	if (details.formats.size() == 1 && details.formats[0].format == vk::Format::eUndefined)
		format = { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear };
	else
	{
		const auto bestFormat = GetBestRatedElement<vk::SurfaceFormatKHR>(details.formats, [](const vk::SurfaceFormatKHR& sf, int index)
		{
			auto score = 0.f;
			if (sf.format == vk::Format::eB8G8R8A8Unorm)
				score += 100.f;
			if (sf.colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear)
				score += 100.f;
			return score;
		});

		if(bestFormat.score > 0)
			format = bestFormat.element;
		else
			format = details.formats[0];
	}

	const auto bestPresentMode = GetBestRatedElement<vk::PresentModeKHR>(details.presentModes, [](const vk::PresentModeKHR& mode, int index)
	{
		switch (mode)
		{
		case vk::PresentModeKHR::eMailbox:
			return 3.f;
		case vk::PresentModeKHR::eFifo:
			return 2.f;
		case vk::PresentModeKHR::eImmediate:
			return 1.f;
		case vk::PresentModeKHR::eFifoRelaxed:
		case vk::PresentModeKHR::eSharedContinuousRefresh:
		case vk::PresentModeKHR::eSharedDemandRefresh:
		default:
			return 0.f;
		}
	});

	vk::Extent2D extend;
	if(details.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		extend = details.capabilities.currentExtent;
	else
	{
		int width, height;
		SDL_GetWindowSize(window, &width, &height);
		const auto minExtent = details.capabilities.minImageExtent;
		const auto maxExtent = details.capabilities.maxImageExtent;
		extend = { std::clamp(minExtent.width, maxExtent.width, static_cast<uint32_t>(width)), std::clamp(minExtent.height, maxExtent.height, static_cast<uint32_t>(height)) };
	}

	auto imageCount = details.capabilities.minImageCount;
	if(bestPresentMode.element == vk::PresentModeKHR::eMailbox)
		imageCount += 1; // use triple buffering for Mailbox Present mode

	if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount)
	    imageCount = details.capabilities.maxImageCount;

	vk::SwapchainCreateInfoKHR swapchainCreateInfo({},
		this->surface, imageCount, format.format, format.colorSpace, extend, 1, vk::ImageUsageFlagBits::eColorAttachment,
		vk::SharingMode::eExclusive, 
		0, nullptr, 
		details.capabilities.currentTransform,
		vk::CompositeAlphaFlagBitsKHR::eOpaque,
		bestPresentMode.element, VK_TRUE, nullptr);

	if(this->queueInfo.graphicsQueueFamilyIndex != this->queueInfo.presentQueueFamilyIndex)
	{
		uint32_t queues[] = { this->queueInfo.graphicsQueueFamilyIndex, this->queueInfo.presentQueueFamilyIndex };
		// TODO: [Performance] convert this into Exclusive mode and handle ownership transfer during rendering
		swapchainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = queues;
	}

	this->swapChain = this->device.createSwapchainKHR(swapchainCreateInfo);
	this->swapChainImages = this->device.getSwapchainImagesKHR(this->swapChain);
}

void VulkanRenderer::Initialize(SDL_Window* window)
{
	this->CreateInstance(window);
	this->CreateSurface(window);
	this->PickPhysicalDevice();
	this->CreateDevice();
	this->CreateSwapChain(window);
}
