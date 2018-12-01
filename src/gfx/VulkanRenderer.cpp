#include "VulkanRenderer.h"
#include <cassert>
#include <set>
#include <vulkan/vulkan.hpp>
#include <SDL_vulkan.h>
#include <spdlog/spdlog.h>
#include "../utils/Rating.hpp"
#include <fstream>

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
	/*if (type == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT && severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		return VK_FALSE;*/

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
	this->device.waitIdle();

	if(this->imageAvailableSemaphore)
	{
		this->device.destroySemaphore(this->imageAvailableSemaphore);
		this->imageAvailableSemaphore = nullptr;
	}

	if(this->renderFinishedSemaphore)
	{
		this->device.destroySemaphore(this->renderFinishedSemaphore);
		this->renderFinishedSemaphore = nullptr;
	}

	if(this->commandPool)
	{
		this->device.destroyCommandPool(this->commandPool);
		this->commandPool = nullptr;
	}

	if(!this->frameBuffers.empty())
	{
		for (auto& frameBuffer : this->frameBuffers)
		{
			this->device.destroyFramebuffer(frameBuffer);
		}
		this->frameBuffers.clear();
	}

	if(this->pipeline)
	{
		this->device.destroyPipeline(this->pipeline);
		this->pipeline = nullptr;
	}

	if(this->pipelineLayout)
	{
		this->device.destroyPipelineLayout(this->pipelineLayout);
		this->pipelineLayout = nullptr;
	}

	if(this->renderPass)
	{
		this->device.destroyRenderPass(this->renderPass);
		this->renderPass = nullptr;
	}

	if (!this->swapChainImageViews.empty())
	{
		for (auto& imageView : this->swapChainImageViews)
		{
			this->device.destroyImageView(imageView);
		}
		this->swapChainImageViews.clear();
	}

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
	auto props = this->physicalDevice.getProperties();
	log->info("Using GPU {0}", props.deviceName);
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
	struct SwapChainSupportDetails
	{
		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR> presentModes;
	};

	SwapChainSupportDetails details;
	details.capabilities = this->physicalDevice.getSurfaceCapabilitiesKHR(this->surface);
	details.formats = this->physicalDevice.getSurfaceFormatsKHR(this->surface);
	details.presentModes = this->physicalDevice.getSurfacePresentModesKHR(this->surface);

	if (details.formats.empty())
		throw std::exception("SwapChain incompatible: no supported surface formats found");

	if (details.presentModes.empty())
		throw std::exception("SwapChain incompatible: no supported present modes found");

	vk::SurfaceFormatKHR surfaceFormat;

	if (details.formats.size() == 1 && details.formats[0].format == vk::Format::eUndefined)
		surfaceFormat = { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear };
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
			surfaceFormat = bestFormat.element;
		else
			surfaceFormat = details.formats[0];
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
		this->surface, imageCount, surfaceFormat.format, surfaceFormat.colorSpace, extend, 1, vk::ImageUsageFlagBits::eColorAttachment,
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

	this->swapChainDetails = { surfaceFormat.format, surfaceFormat.colorSpace, bestPresentMode.element, extend };

	for (auto& image : this->swapChainImages)
	{
		vk::ImageViewCreateInfo createInfo({}, image, vk::ImageViewType::e2D, surfaceFormat.format, {}, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
		this->swapChainImageViews.emplace_back(this->device.createImageView(createInfo));
	}
}

void VulkanRenderer::CreateRenderPass()
{
	vk::AttachmentDescription colorAttachment({}, this->swapChainDetails.format,
		vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
		vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
		vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR);

	vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);
	vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorAttachmentRef);
	vk::SubpassDependency subpassDependency(
		VK_SUBPASS_EXTERNAL, 0,
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
		{}, vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

	const vk::RenderPassCreateInfo createInfo({}, 1, &colorAttachment, 1, &subpass, 1, &subpassDependency);
	this->renderPass = this->device.createRenderPass(createInfo);
}

static std::vector<char> readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if(file.fail())
		throw std::exception(std::string("unable to open " + filename).c_str());

	const auto fileSize = file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();
	return buffer;
}

static vk::ShaderModule createShaderModule(const std::vector<char>& code, const vk::Device& device)
{
	const vk::ShaderModuleCreateInfo createInfo({}, code.size(), reinterpret_cast<const uint32_t*>(code.data()));
	return device.createShaderModule(createInfo);
}

void VulkanRenderer::CreateGraphicsPipeline()
{
	const auto vertShader = createShaderModule(readFile("shader/triangle.vert.spv"), this->device);
	const auto fragShader = createShaderModule(readFile("shader/triangle.frag.spv"), this->device);

	const vk::PipelineShaderStageCreateInfo vertShaderStageInfo({}, vk::ShaderStageFlagBits::eVertex, vertShader, "main");
	const vk::PipelineShaderStageCreateInfo fragShaderStageInfo({}, vk::ShaderStageFlagBits::eFragment, fragShader, "main");

	vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo({},  0, nullptr, 0, nullptr);
	vk::PipelineInputAssemblyStateCreateInfo inputAssembly({}, vk::PrimitiveTopology::eTriangleList, VK_FALSE);
	auto width = static_cast<float>(this->swapChainDetails.extent.width), height = static_cast<float>(this->swapChainDetails.extent.height);
	vk::Viewport viewport(0, 0, width, height, 0.f, 1.f);
	vk::Rect2D scissor({0, 0}, this->swapChainDetails.extent);
	vk::PipelineViewportStateCreateInfo viewportState({}, 1, &viewport, 1, &scissor);
	vk::PipelineRasterizationStateCreateInfo rasterizerState({}, VK_FALSE, VK_FALSE, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise, 0, 0, 0, 0, 1.f);
	// enabling multisampling requires a GPU logical device feature to be enabled during creation
	vk::PipelineMultisampleStateCreateInfo multisampleState({}, vk::SampleCountFlagBits::e1, VK_FALSE, 1.f, nullptr, VK_FALSE, VK_FALSE);
	//vk::PipelineDepthStencilStateCreateInfo depthStencilInfo({});
	vk::PipelineColorBlendAttachmentState colorBlendAttachment(VK_TRUE, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd, vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
	vk::PipelineColorBlendStateCreateInfo colorBlending({}, VK_FALSE, vk::LogicOp::eCopy, 1, &colorBlendAttachment);

	vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo({}, 0, nullptr, 0, nullptr);
	this->pipelineLayout = this->device.createPipelineLayout(pipelineLayoutCreateInfo);

	vk::GraphicsPipelineCreateInfo pipelineCreateInfo({}, 2, shaderStages, &vertexInputInfo, &inputAssembly, nullptr, &viewportState, &rasterizerState, &multisampleState, nullptr, &colorBlending, nullptr, this->pipelineLayout, this->renderPass, 0, nullptr, -1);
	this->pipeline = this->device.createGraphicsPipelines(nullptr, vk::ArrayProxy<const vk::GraphicsPipelineCreateInfo>(1, &pipelineCreateInfo))[0];

	this->device.destroyShaderModule(vertShader);
	this->device.destroyShaderModule(fragShader);
}

void VulkanRenderer::CreateFrameBuffers()
{
	for (auto& swapChainImageView : this->swapChainImageViews)
	{
		const auto extent = this->swapChainDetails.extent;
		vk::FramebufferCreateInfo framebufferCreateInfo({}, this->renderPass, 1, &swapChainImageView, extent.width, extent.height, 1);

		this->frameBuffers.emplace_back(this->device.createFramebuffer(framebufferCreateInfo));
	}
}

void VulkanRenderer::CreateCommandPool()
{
	const vk::CommandPoolCreateInfo createInfo({}, this->queueInfo.graphicsQueueFamilyIndex);
	this->commandPool = this->device.createCommandPool(createInfo);
}

void VulkanRenderer::CreateCommandBuffers()
{
	const vk::CommandBufferAllocateInfo allocateInfo(this->commandPool, vk::CommandBufferLevel::ePrimary, this->frameBuffers.size());
	this->commandBuffers = this->device.allocateCommandBuffers(allocateInfo);

	for(auto i = 0; i < this->commandBuffers.size(); i++)
	{
		auto commandBuffer = this->commandBuffers[i];
		vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse, nullptr);
		commandBuffer.begin(beginInfo);

		vk::ClearValue clearValue[] = 
		{
			vk::ClearColorValue(std::array<float, 4> { 0.f, 0.f, 0.f, 1.f })
		};
		vk::RenderPassBeginInfo renderPassInfo(this->renderPass, this->frameBuffers[i], vk::Rect2D({0, 0}, this->swapChainDetails.extent), 1, clearValue);
		commandBuffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, this->pipeline);

		commandBuffer.draw(3, 1, 0, 0);

		commandBuffer.endRenderPass();
		commandBuffer.end();
	}
}

void VulkanRenderer::CreateSemaphores()
{
	this->imageAvailableSemaphore = this->device.createSemaphore({});
	this->renderFinishedSemaphore = this->device.createSemaphore({});
}

void VulkanRenderer::Initialize(SDL_Window* window)
{
	this->CreateInstance(window);
	this->CreateSurface(window);
	this->PickPhysicalDevice();
	this->CreateDevice();
	this->CreateSwapChain(window);
	this->CreateRenderPass();
	this->CreateGraphicsPipeline();
	this->CreateFrameBuffers();
	this->CreateCommandPool();
	this->CreateCommandBuffers();
	this->CreateSemaphores();
}

void VulkanRenderer::Draw()
{
	const auto imageResult = this->device.acquireNextImageKHR(this->swapChain, std::numeric_limits<uint64_t>::max(), this->imageAvailableSemaphore, nullptr);
	if(imageResult.result != vk::Result::eSuccess)
		throw std::exception("Unable to retrieve image from swap chain");

	vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
	vk::SubmitInfo submitInfo(1, &this->imageAvailableSemaphore, waitStages, 1, &this->commandBuffers[imageResult.value], 1, &this->renderFinishedSemaphore);

	if(this->queueInfo.graphicsQueue.submit(1, &submitInfo, nullptr) != vk::Result::eSuccess)
		throw std::exception("error while submitting command buffer to graphics queue");

	vk::PresentInfoKHR presentInfo(1, &this->renderFinishedSemaphore, 1, &this->swapChain, &imageResult.value);
	if(this->queueInfo.presentQueue.presentKHR(presentInfo) != vk::Result::eSuccess)
		throw std::exception("Error presenting next frame");

	this->queueInfo.presentQueue.waitIdle();
}
