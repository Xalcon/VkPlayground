#include <iostream>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <spdlog/spdlog.h>
#include "spdlog/sinks/stdout_color_sinks.h"

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
		SDL_WINDOW_SHOWN
	);

	if (window == nullptr) {
		log->error("could not create sdl2 window: {0}", SDL_GetError());
		return EXIT_FAILURE;
	}

	log->info("SDL2 window created");
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