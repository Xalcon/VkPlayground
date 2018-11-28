#pragma once
#include <SDL.h>
#include <vulkan/vulkan.hpp>
#include "gfx/IRenderer.hpp"
#include <functional>

class App
{
	SDL_Window* window;
	std::unique_ptr<IRenderer> renderer;

	void InitWindow();
	void MainLoop();
	void Cleanup();

public:
	App(std::function<std::unique_ptr<IRenderer>()> renderFactory);
	~App();

	void Run();
};

