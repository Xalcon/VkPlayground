#pragma once
#include <string>
#include <SDL.h>

namespace vkp::tools
{
	std::string sdlEventToString(SDL_EventType event)
	{
		switch (event)
		{
#define STR(r) case SDL_ ##r: return #r
			STR(QUIT);

			STR(APP_TERMINATING);
		    STR(APP_LOWMEMORY);
		    STR(APP_WILLENTERBACKGROUND);
		    STR(APP_DIDENTERBACKGROUND);
		    STR(APP_WILLENTERFOREGROUND);
		    STR(APP_DIDENTERFOREGROUND);
		    STR(DISPLAYEVENT);
		    STR(WINDOWEVENT);
		    STR(SYSWMEVENT);

		    /* Keyboard events */
		    STR(KEYDOWN);
		    STR(KEYUP);
		    STR(TEXTEDITING);
		    STR(TEXTINPUT);
		    STR(KEYMAPCHANGED);

		    /* Mouse events */
		    STR(MOUSEMOTION);
		    STR(MOUSEBUTTONDOWN);
		    STR(MOUSEBUTTONUP);
		    STR(MOUSEWHEEL);

		    /* Joystick events */
		    STR(JOYAXISMOTION);
		    STR(JOYBALLMOTION);
		    STR(JOYHATMOTION);
		    STR(JOYBUTTONDOWN);
		    STR(JOYBUTTONUP);
		    STR(JOYDEVICEADDED);
		    STR(JOYDEVICEREMOVED);

		    /* Game controller events */
		    STR(CONTROLLERAXISMOTION);
		    STR(CONTROLLERBUTTONDOWN);
		    STR(CONTROLLERBUTTONUP);
		    STR(CONTROLLERDEVICEADDED);
		    STR(CONTROLLERDEVICEREMOVED);
		    STR(CONTROLLERDEVICEREMAPPED);

		    /* Touch events */
		    STR(FINGERDOWN);
		    STR(FINGERUP);
		    STR(FINGERMOTION);

		    /* Gesture events */
		    STR(DOLLARGESTURE);
		    STR(DOLLARRECORD);
		    STR(MULTIGESTURE);

		    /* Clipboard events */
		    STR(CLIPBOARDUPDATE);

		    /* Drag and drop events */
		    STR(DROPFILE);
		    STR(DROPTEXT);
		    STR(DROPBEGIN);
		    STR(DROPCOMPLETE);

		    /* Audio hotplug events */
		    STR(AUDIODEVICEADDED);
		    STR(AUDIODEVICEREMOVED);

		    /* Sensor events */
		    STR(SENSORUPDATE);

		    /* Render events */
		    STR(RENDER_TARGETS_RESET);
		    STR(RENDER_DEVICE_RESET);
#undef STR
		default:
			return fmt::format("UNKNOWN_SDL_EVENT_{0:04X}", event);
		}
	}

	std::string sdlWindowEventToString(uint8_t event)
	{
		switch (event)
		{
#define STR(r) case SDL_WINDOWEVENT_ ##r: return #r
			STR(NONE);           /**< Never used */
		    STR(SHOWN);          /**< Window has been shown */
		    STR(HIDDEN);         /**< Window has been hidden */
		    STR(EXPOSED);        /**< Window has been exposed and should be redrawn */
		    STR(MOVED);          /**< Window has been moved to data1, data2 */
		    STR(RESIZED);        /**< Window has been resized to data1xdata2 */
		    STR(SIZE_CHANGED);   /**< The window size has changed, either as a result of an API call or through the system or user changing the window size. */
		    STR(MINIMIZED);      /**< Window has been minimized */
		    STR(MAXIMIZED);      /**< Window has been maximized */
		    STR(RESTORED);       /**< Window has been restored to normal size and position */
		    STR(ENTER);          /**< Window has gained mouse focus */
		    STR(LEAVE);         /**< Window has lost mouse focus */
		    STR(FOCUS_GAINED);   /**< Window has gained keyboard focus */
		    STR(FOCUS_LOST);     /**< Window has lost keyboard focus */
		    STR(CLOSE);          /**< The window manager requests that the window be closed */
		    STR(TAKE_FOCUS);     /**< Window is being offered a focus (should SetWindowInputFocus() on itself or a subwindow, or ignore) */
		    STR(HIT_TEST);        /**< Window had a hit test that wasn't SDL_HITTEST_NORMAL. */
#undef STR
		default:
			return fmt::format("UNKNOWN_SDL_EVENT_{0:04X}", event);
		}
	}
}
