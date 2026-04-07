module;
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
export module SDL.Renderer;

export namespace SDL
{
	class Renderer
	{
		SDL_Renderer* m_renderer{};
	public:
		Renderer() = default;

		Renderer(SDL_Window* window, int index, Uint32 flags) : m_renderer{ SDL_CreateRenderer(window, index, flags) }
		{
		}

		void Create(SDL_Window* window, int index, Uint32 flags)
		{
			m_renderer = SDL_CreateRenderer(window, index, flags);
		}

		void Destroy()
		{
			if(m_renderer)
				SDL_DestroyRenderer(m_renderer);
		}

		constexpr SDL_Renderer* Get() const
		{
			return m_renderer;
		}

		~Renderer()
		{
			Destroy();
		}
	};
}