module;
#include <SDL2/SDL_video.h>
#include <utility>
export module SDL.Window;


export namespace SDL
{
	class Window
	{
		SDL_Window* m_window{};
	public:
		Window() = default;

		Window(const char* title, int x, int y, int w, int h, Uint32 flags) :
			m_window{ SDL_CreateWindow(title, x, y, w, h, flags) }
		{
		}

		void Create(const char* title, int x, int y, int w, int h, Uint32 flags)
		{
			m_window = SDL_CreateWindow(title, x, y, w, h, flags);
		}

		void SetFullscreen(bool fullscreen)
		{
			SDL_SetWindowFullscreen(m_window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
		}

		std::pair<int, int> GetSize() const
		{
			std::pair<int, int> result;
			SDL_GetWindowSize(m_window, &result.first, &result.second);
			return result;
		}

		constexpr SDL_Window* Get() const
		{
			return m_window;
		}

		void Destroy()
		{
			if (m_window)
				SDL_DestroyWindow(m_window);
		}

		~Window()
		{
			Destroy();
		}
	};
}