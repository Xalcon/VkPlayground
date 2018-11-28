#include "App.hpp"
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>
#include <SDL_vulkan.h>


App::App(std::function<std::unique_ptr<IRenderer>()> renderFactory)
{
	this->renderer = renderFactory();
}


App::~App()
{
	this->Cleanup();
}

void App::InitWindow()
{
	auto log = spdlog::get("logger");
	this->window = SDL_CreateWindow(
		"VkPlayground",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		800, 480,
		SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN
	);

	if (this->window == nullptr)
		throw std::exception(SDL_GetError());

	log->info("SDL2 window created");

	const auto screenSurface = SDL_GetWindowSurface(this->window);
	SDL_FillRect(screenSurface, nullptr, SDL_MapRGB(screenSurface->format, 0x44, 0x44, 0x44));
	SDL_UpdateWindowSurface(this->window);
}

void App::MainLoop()
{
	auto log = spdlog::get("logger");
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
}

void App::Cleanup()
{
	if (this->window)
	{
		SDL_DestroyWindow(this->window);
		this->window = nullptr;
	}
}

void App::Run()
{
	this->InitWindow();
	this->renderer->Initialize(this->window);
	this->MainLoop();
	this->Cleanup();
}
