#pragma once
#include <SDL.h>

class IRenderer
{
public:
	IRenderer() {}
	virtual ~IRenderer() { };

	virtual void Initialize(SDL_Window* window) = 0;
	virtual void Draw() = 0;
};

