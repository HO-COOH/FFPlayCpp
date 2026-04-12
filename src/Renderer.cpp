module;
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <SDL2/SDL_render.h>
extern "C" {
#include <libavutil/pixfmt.h>
#include <libavutil/rational.h>
#include <libavutil/time.h>
#include <libavutil/mathematics.h>
}
module ffplay;
import :Renderer;
import utils.Frame;
import AV.Rational;

constexpr int ceil_rshift(int value, int shift)
{
	return (value + (1 << shift) - 1) >> shift;
}

void calculate_display_rect(SDL_Rect& rect, int scr_xleft, int scr_ytop, int scr_width, int scr_height, Frame const& frame)
{
	auto aspect_ratio = frame.sample_aspect_ratio;
	if (aspect_ratio <= AV::Rational{ 0, 1 })
		aspect_ratio = AV::Rational{ 1,1 };

	aspect_ratio = aspect_ratio * AV::Rational{ frame.width, frame.height };

	int64_t height = scr_height;
	int64_t width = av_rescale(height, aspect_ratio.num, aspect_ratio.den) & ~1;
	if (width > scr_width)
	{
		width = scr_width;
		height = av_rescale(width, aspect_ratio.den, aspect_ratio.num) & ~1;
	}

	rect.x = scr_xleft + static_cast<int>((scr_width - width) / 2);
	rect.y = scr_ytop + static_cast<int>((scr_height - height) / 2);
	rect.w = std::max(static_cast<int>(width), 1);
	rect.h = std::max(static_cast<int>(height), 1);
}

constexpr SDL_BlendMode avFormatToSDLBlendMode(int avFormat)
{
	switch (avFormat)
	{
		case AV_PIX_FMT_RGB32:
		case AV_PIX_FMT_RGB32_1:
		case AV_PIX_FMT_BGR32:
		case AV_PIX_FMT_BGR32_1:
			return SDL_BlendMode::SDL_BLENDMODE_BLEND;
		default:
			return SDL_BlendMode::SDL_BLENDMODE_NONE;
	}
}

constexpr Uint32 avFormatToSDLPixelFormat(int avFormat)
{
	switch (avFormat)
	{
		case AV_PIX_FMT_YUV420P:
		case AV_PIX_FMT_YUVJ420P:
			return SDL_PixelFormatEnum::SDL_PIXELFORMAT_IYUV;
		case AV_PIX_FMT_YUYV422:
			return SDL_PixelFormatEnum::SDL_PIXELFORMAT_YUY2;
		case AV_PIX_FMT_UYVY422:
			return SDL_PixelFormatEnum::SDL_PIXELFORMAT_UYVY;
		case AV_PIX_FMT_RGB24:
			return SDL_PixelFormatEnum::SDL_PIXELFORMAT_RGB24;
		case AV_PIX_FMT_BGR24:
			return SDL_PixelFormatEnum::SDL_PIXELFORMAT_BGR24;
		case AV_PIX_FMT_ARGB:
			return SDL_PixelFormatEnum::SDL_PIXELFORMAT_ARGB8888;
		case AV_PIX_FMT_RGBA:
			return SDL_PixelFormatEnum::SDL_PIXELFORMAT_RGBA8888;
		case AV_PIX_FMT_ABGR:
			return SDL_PixelFormatEnum::SDL_PIXELFORMAT_ABGR8888;
		case AV_PIX_FMT_BGRA:
			return SDL_PixelFormatEnum::SDL_PIXELFORMAT_BGRA8888;
		default:
			return SDL_PixelFormatEnum::SDL_PIXELFORMAT_UNKNOWN;
	}
}


bool Renderer::recreate_texture(Uint32 format, int width, int height, SDL_BlendMode blend_mode)
{
	bool recreate = m_videoTexture.Get() == nullptr;
	if (!recreate)
	{
		auto const attribute = m_videoTexture.Query();
		recreate = attribute.format != format || attribute.width != width || attribute.height != height;
	}

	if (recreate)
	{
		m_videoTexture = SDL::Texture{ m_renderer.Get(), format, SDL_TEXTUREACCESS_STREAMING, width, height };
		if (!m_videoTexture.Get())
			return false;

		if (SDL_SetTextureBlendMode(m_videoTexture.Get(), blend_mode) < 0)
			return false;
	}

	return true;
}

bool Renderer::upload_texture(Frame& frame)
{
	Uint32 const sdl_pix_fmt = avFormatToSDLPixelFormat(frame.frame->format);
	SDL_BlendMode const sdl_blendmode = avFormatToSDLBlendMode(frame.frame->format);
	if (sdl_pix_fmt == SDL_PIXELFORMAT_UNKNOWN)
		return false;

	if (!recreate_texture(sdl_pix_fmt, frame.frame->width, frame.frame->height, sdl_blendmode))
		return false;

	int ret = 0;
	auto* texture = m_videoTexture.Get();
	switch (sdl_pix_fmt)
	{
		case SDL_PIXELFORMAT_IYUV:
			if (frame.frame->linesize[0] > 0 && frame.frame->linesize[1] > 0 && frame.frame->linesize[2] > 0)
			{
				ret = SDL_UpdateYUVTexture(texture, nullptr,
					frame.frame->data[0], frame.frame->linesize[0],
					frame.frame->data[1], frame.frame->linesize[1],
					frame.frame->data[2], frame.frame->linesize[2]);
			}
			else if (frame.frame->linesize[0] < 0 && frame.frame->linesize[1] < 0 && frame.frame->linesize[2] < 0)
			{
				ret = SDL_UpdateYUVTexture(texture, nullptr,
					frame.frame->data[0] + frame.frame->linesize[0] * (frame.frame->height - 1), -frame.frame->linesize[0],
					frame.frame->data[1] + frame.frame->linesize[1] * (ceil_rshift(frame.frame->height, 1) - 1), -frame.frame->linesize[1],
					frame.frame->data[2] + frame.frame->linesize[2] * (ceil_rshift(frame.frame->height, 1) - 1), -frame.frame->linesize[2]);
			}
			else
			{
				return false;
			}
			break;
		default:
			if (frame.frame->linesize[0] < 0)
			{
				ret = SDL_UpdateTexture(texture, nullptr,
					frame.frame->data[0] + frame.frame->linesize[0] * (frame.frame->height - 1),
					-frame.frame->linesize[0]);
			}
			else
			{
				ret = SDL_UpdateTexture(texture, nullptr, frame.frame->data[0], frame.frame->linesize[0]);
			}
			break;
	}

	frame.uploaded = ret == 0;
	return ret == 0;
}

void Renderer::Clear()
{
	SDL_SetRenderDrawColor(m_renderer.Get(), 0, 0, 0, 255);
	SDL_RenderClear(m_renderer.Get());
}

void Renderer::Display(SDL_Window* window, Frame& vp)
{
	if (!m_renderer.Get())
		return;

	SDL_Rect rect{};
	int width{};
	int height{};
	if (SDL_GetRendererOutputSize(m_renderer.Get(), &width, &height) < 0 || width <= 0 || height <= 0)
		SDL_GetWindowSize(window, &width, &height);

	calculate_display_rect(rect, 0, 0, width, height, vp);
	if (!vp.uploaded)
		upload_texture(vp);

	if (vp.uploaded && m_videoTexture.Get())
	{
		SDL_RenderCopyEx(
			m_renderer.Get(),
			m_videoTexture.Get(),
			nullptr,
			&rect,
			0.0,
			nullptr,
			vp.flip_v() ? SDL_FLIP_VERTICAL : SDL_FLIP_NONE
		);
	}
}

void Renderer::Present()
{
	if (m_renderer.Get())
		SDL_RenderPresent(m_renderer.Get());
}