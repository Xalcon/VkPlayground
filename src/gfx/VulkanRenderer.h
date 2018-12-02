#pragma once
#include "IRenderer.hpp"
#include <vulkan/vulkan.hpp>

struct QueueInfo
{
	uint32_t graphicsQueueFamilyIndex;
	vk::Queue graphicsQueue;
	uint32_t presentQueueFamilyIndex;
	vk::Queue presentQueue;
};

struct SwapChainDetails
{
	vk::Format format;
	vk::ColorSpaceKHR colorSpace;
	vk::PresentModeKHR presentMode;
	vk::Extent2D extent;
};

class VulkanRenderer :
	public IRenderer
{
    vk::Instance instance;
	VkDebugUtilsMessengerEXT callback;
	vk::PhysicalDevice physicalDevice;
	vk::Device device;
	vk::SurfaceKHR surface;
	vk::SwapchainKHR swapChain;
	std::vector<vk::Image> swapChainImages;
	std::vector<vk::ImageView> swapChainImageViews;
	vk::RenderPass renderPass;
	vk::PipelineLayout pipelineLayout;
	vk::Pipeline pipeline;
	std::vector<vk::Framebuffer> frameBuffers;
	vk::CommandPool commandPool;
	std::vector<vk::CommandBuffer> commandBuffers;
	std::vector<vk::Semaphore> imageAvailableSemaphores;
	std::vector<vk::Semaphore> renderFinishedSemaphores;
	std::vector<vk::Fence> inFlightFences;
	
	QueueInfo queueInfo = {};
	SwapChainDetails swapChainDetails = {};
	int currentFrame = 0;
	
	void RegisterDebugCallback();
	void DestroyDebugCallback();
	
	void CreateInstance(SDL_Window* window);
	void CreateSurface(SDL_Window* window);
	void PickPhysicalDevice();
	void CreateDevice();
	void CreateSwapChain(SDL_Window* window);
	void CreateRenderPass();
	void CreateGraphicsPipeline();
	void CreateFrameBuffers();
	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateSyncObjects();

	void CleanupSwapChain();
public:
	VulkanRenderer();
	virtual ~VulkanRenderer();
	void Initialize(SDL_Window* window) override;
	void Draw() override;
};

