module;
#include <SDL2/SDL_video.h>
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