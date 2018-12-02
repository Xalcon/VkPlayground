#include <vulkan/vulkan.hpp>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "App.hpp"
#include "gfx/VulkanRenderer.h"

void setupLogging()
{
	spdlog::stdout_color_mt("logger")->set_level(spdlog::level::level_enum::trace);
	spdlog::stdout_color_mt("vk-perf")->set_level(spdlog::level::level_enum::trace);
	spdlog::stdout_color_mt("vk-general")->set_level(spdlog::level::level_enum::trace);
	spdlog::stdout_color_mt("vk-val")->set_level(spdlog::level::level_enum::trace);
}

int main(int argc, const char* argv[])
{
	setupLogging();
	auto log = spdlog::get("logger");

	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		throw std::exception(SDL_GetError());

	try
	{
		App app([]() { return std::make_unique<VulkanRenderer>(); });
		app.Run();
	}
	catch(const std::exception& e)
	{
		log->error("Error initializing vulkan: {0}", e.what());
		return EXIT_FAILURE;
	}

	
	log->info("shutting down...");
	SDL_Quit();
	return EXIT_SUCCESS;
}