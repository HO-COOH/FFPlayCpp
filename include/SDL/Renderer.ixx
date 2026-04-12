module;
#include <utility>
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
		Renderer(Renderer const&) = delete;
		Renderer(Renderer&& other) noexcept : m_renderer{ std::exchange(other.m_renderer, nullptr) }
		{
		}
		Renderer& operator=(Renderer const&) = delete;
		Renderer& operator=(Renderer&& other) noexcept
		{
			if (this != &other)
			{
				Destroy();
				m_renderer = std::exchange(other.m_renderer, nullptr);
			}
			return *this;
		}

		Renderer(SDL_Window* window, int index, Uint32 flags) : m_renderer{ SDL_CreateRenderer(window, index, flags) }
		{
		}

		void Create(SDL_Window* window, int index, Uint32 flags)
		{
			Destroy();
			m_renderer = SDL_CreateRenderer(window, index, flags);
		}

		std::pair<int, int> GetOutputSize()
		{
			std::pair<int, int> result{};
			SDL_GetRendererOutputSize(m_renderer, &result.first, &result.second);
			return result;
		}

		void Destroy()
		{
			if(m_renderer)
			{
				SDL_DestroyRenderer(m_renderer);
				m_renderer = nullptr;
			}
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