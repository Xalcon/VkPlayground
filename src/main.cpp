#include <iostream>
#define SDL_MAIN_HANDLED
#include <SDL.h>

int main(int argc, const char* argv[])
{
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "could not initialize sdl2: %s\n", SDL_GetError());
		return EXIT_FAILURE;
	}

	const auto window = SDL_CreateWindow(
		"hello_sdl2",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		800, 480,
		SDL_WINDOW_SHOWN
	);

	if (window == nullptr) {
		fprintf(stderr, "could not create window: %s\n", SDL_GetError());
		return EXIT_FAILURE;
	}

	const auto screenSurface = SDL_GetWindowSurface(window);
	SDL_FillRect(screenSurface, nullptr, SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF));
	SDL_UpdateWindowSurface(window);
	SDL_SetWindowTitle(window, "VkPlayground");
	//e is an SDL_Event variable we've declared before entering the main loop
	SDL_Event e;
	bool quit = false;
	while (!quit) {
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				quit = true;
			}
			if (e.type == SDL_KEYDOWN) {
				quit = true;
			}
			if (e.type == SDL_MOUSEBUTTONDOWN) {
				quit = true;
			}
		}
		//Render the scene
	}
	SDL_DestroyWindow(window);
	SDL_Quit();
	return EXIT_SUCCESS;
}