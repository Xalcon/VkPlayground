#include <vulkan/vulkan.hpp>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_vulkan.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

vk::Instance instance;

void initVulkan(SDL_Window* window)
{
	auto log = spdlog::get("logger");
	log->info("Initializing vulkan instance");

	vk::ApplicationInfo appInfo("VkPlayground", VK_MAKE_VERSION(0, 1, 0), "unnamed", VK_MAKE_VERSION(0, 1, 0), VK_API_VERSION_1_1);

	unsigned int extCount = 0;
	if(!SDL_Vulkan_GetInstanceExtensions(window, &extCount, nullptr))
	{
		const auto error = SDL_GetError();
		log->error("Error getting instance extensions: {0}", error);
		return;
	}

	std::vector<const char*> extensions = {};
	unsigned int totalExtensionCount = extCount + extensions.size();
	extensions.resize(totalExtensionCount);

	std::vector<const char*> layers = {};
	const unsigned int totalLayerCount = layers.size();
	
	if (!SDL_Vulkan_GetInstanceExtensions(window, &totalExtensionCount, extensions.data()))
		throw std::exception("unable to get SDL instance extensions");

	for (auto& extension : extensions)
		log->info("Requesting extension {0}", extension);

	instance = vk::createInstance(vk::InstanceCreateInfo(vk::InstanceCreateFlags(), &appInfo, totalLayerCount, layers.data(), extensions.size(), extensions.data()));
	assert(instance);
}

void setupLogging()
{
	auto console = spdlog::stdout_color_mt("logger");
}

int main(int argc, const char* argv[])
{
	setupLogging();
	auto log = spdlog::get("logger");

	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		log->error("could not initialize sdl2: {0}", SDL_GetError());
		return EXIT_FAILURE;
	}

	const auto window = SDL_CreateWindow(
		"hello_sdl2",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		800, 480,
		SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN
	);

	if (window == nullptr) {
		log->error("could not create sdl2 window: {0}", SDL_GetError());
		return EXIT_FAILURE;
	}
	log->info("SDL2 window created");

	try
	{
		initVulkan(window);
	}
	catch(const std::exception& e)
	{
		log->error("Error initializing vulkan: {0}", e.what());
		return EXIT_FAILURE;
	}


	const auto screenSurface = SDL_GetWindowSurface(window);
	SDL_FillRect(screenSurface, nullptr, SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF));
	SDL_UpdateWindowSurface(window);
	SDL_SetWindowTitle(window, "VkPlayground");
	//e is an SDL_Event variable we've declared before entering the main loop
	log->info("Entering main loop");
	SDL_Event e;
	bool quit = false;
	while (!quit) {
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				quit = true;
			}
			if (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
				quit = true;
			}
		}
		//Render the scene
	}
	log->info("shutting down...");
	SDL_DestroyWindow(window);
	SDL_Quit();
	return EXIT_SUCCESS;
}