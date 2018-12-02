#pragma once
#include <SDL.h>

class IRenderer
{
public:
	IRenderer() {}
	virtual ~IRenderer() { };

	virtual void Initialize(SDL_Window* window) = 0;
	virtual void Resize(SDL_Window* window, uint32_t width, uint32_t height) = 0;
	virtual void Draw() = 0;
};

