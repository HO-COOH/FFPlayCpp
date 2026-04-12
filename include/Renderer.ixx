module;
#include <utility>
#include <SDL2/SDL_render.h>
export module ffplay:Renderer;

import utils.Frame;
import SDL.Texure;
import SDL.Renderer;

export class Renderer
{
	SDL::Renderer m_renderer;
	SDL::Texture m_videoTexture;

	bool recreate_texture(Uint32 format, int width, int height, SDL_BlendMode blend_mode);
	bool upload_texture(Frame& frame);

public:
	Renderer() = default;
	Renderer(Renderer const&) = delete;
	Renderer(Renderer&& other) noexcept = default;
	Renderer& operator=(Renderer const&) = delete;
	Renderer& operator=(Renderer&& other) noexcept
	{
		if (this != &other)
		{
			m_renderer = std::move(other.m_renderer);
			m_videoTexture = std::move(other.m_videoTexture);
		}
		return *this;
	}
	Renderer(SDL_Window* window, int index, Uint32 flags) : m_renderer{ window, index, flags }
	{
	}

	void Clear();
	void Display(SDL_Window* window, Frame& vp);
	void Present();
};