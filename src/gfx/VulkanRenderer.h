#pragma once
#include "IRenderer.hpp"
#include <vulkan/vulkan.hpp>

struct QueueInfo
{
	vk::Queue graphicsQueue;
	vk::Queue presentQueue;
};

class VulkanRenderer :
	public IRenderer
{
    vk::Instance instance;
	VkDebugUtilsMessengerEXT callback;
	vk::PhysicalDevice physicalDevice;
	vk::Device device;
	vk::SurfaceKHR surface;

	QueueInfo queueInfo = {};

	void RegisterDebugCallback();
	void DestroyDebugCallback();
	
	void CreateInstance(SDL_Window* window);
	void CreateSurface(SDL_Window* window);
	void PickPhysicalDevice();
	void CreateDevice();

public:
	VulkanRenderer();
	virtual ~VulkanRenderer();
	void Initialize(SDL_Window* window) override;
};

