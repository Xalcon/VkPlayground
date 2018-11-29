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

struct SwapChainSupportDetails
{
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> presentModes;
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

	QueueInfo queueInfo = {};
	
	void RegisterDebugCallback();
	void DestroyDebugCallback();
	
	void CreateInstance(SDL_Window* window);
	void CreateSurface(SDL_Window* window);
	void PickPhysicalDevice();
	void CreateDevice();
	void CreateSwapChain(SDL_Window* window);

public:
	VulkanRenderer();
	virtual ~VulkanRenderer();
	void Initialize(SDL_Window* window) override;
};

