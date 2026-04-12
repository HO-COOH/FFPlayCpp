module;
#include <SDL2/SDL_render.h>
#include <utility>
export module SDL.Texure;

export namespace SDL
{
	class Texture
	{
		SDL_Texture* m_texture{};
	public:
		struct TextureAttribute
		{
			Uint32 format;
			int width;
			int height;
		};

		Texture() = default;
		Texture(Texture const&) = delete;
		constexpr Texture(Texture&& rhs) noexcept : m_texture{ std::exchange(rhs.m_texture, nullptr) } {}
		Texture& operator=(Texture const&) = delete;

		Texture(SDL_Renderer* renderer, Uint32 format, int access, int width, int height) : m_texture{ SDL_CreateTexture(renderer, format, access, width, height) }
		{
		}

		Texture& operator=(Texture&& rhs) noexcept
		{
			if (this != &rhs)
			{
				Destroy();
				m_texture = std::exchange(rhs.m_texture, nullptr);
			}
			return *this;
		}

		void Destroy()
		{
			if (m_texture)
			{
				SDL_DestroyTexture(m_texture);
				m_texture = nullptr;
			}
		}

		~Texture()
		{
			Destroy();
		}

		constexpr SDL_Texture* Get() const
		{
			return m_texture;
		}

		TextureAttribute Query() const
		{
			TextureAttribute attribute{};
			auto const ret = SDL_QueryTexture(m_texture, &attribute.format, nullptr, &attribute.width, &attribute.height);
			return attribute;
		}
	};


}