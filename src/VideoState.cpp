module;
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <mutex>
extern "C" {
#include <libavutil/pixfmt.h>
#include <libavutil/rational.h>
#include <libavutil/time.h>
#include <libavutil/mathematics.h>
}
module ffplay;
import :VideoState;
import :Options;
import utils.Clock;

namespace
{
	constexpr int ceil_rshift(int value, int shift)
	{
		return (value + (1 << shift) - 1) >> shift;
	}

	void calculate_display_rect(SDL_Rect& rect, int scr_xleft, int scr_ytop, int scr_width, int scr_height, Frame const& frame)
	{
		auto aspect_ratio = frame.sample_aspect_ratio;
		if (av_cmp_q(aspect_ratio, av_make_q(0, 1)) <= 0)
			aspect_ratio = av_make_q(1, 1);

		aspect_ratio = av_mul_q(aspect_ratio, av_make_q(frame.width, frame.height));

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

	void get_sdl_pix_fmt_and_blendmode(int format, Uint32& sdl_pix_fmt, SDL_BlendMode& sdl_blendmode)
	{
		sdl_blendmode = SDL_BLENDMODE_NONE;
		sdl_pix_fmt = SDL_PIXELFORMAT_UNKNOWN;

		switch (format)
		{
			case AV_PIX_FMT_RGB32:
			case AV_PIX_FMT_RGB32_1:
			case AV_PIX_FMT_BGR32:
			case AV_PIX_FMT_BGR32_1:
				sdl_blendmode = SDL_BLENDMODE_BLEND;
				break;
			default:
				break;
		}

		switch (format)
		{
			case AV_PIX_FMT_YUV420P:
			case AV_PIX_FMT_YUVJ420P:
				sdl_pix_fmt = SDL_PIXELFORMAT_IYUV;
				return;
			case AV_PIX_FMT_YUYV422:
				sdl_pix_fmt = SDL_PIXELFORMAT_YUY2;
				return;
			case AV_PIX_FMT_UYVY422:
				sdl_pix_fmt = SDL_PIXELFORMAT_UYVY;
				return;
			case AV_PIX_FMT_RGB24:
				sdl_pix_fmt = SDL_PIXELFORMAT_RGB24;
				return;
			case AV_PIX_FMT_BGR24:
				sdl_pix_fmt = SDL_PIXELFORMAT_BGR24;
				return;
			case AV_PIX_FMT_ARGB:
				sdl_pix_fmt = SDL_PIXELFORMAT_ARGB8888;
				return;
			case AV_PIX_FMT_RGBA:
				sdl_pix_fmt = SDL_PIXELFORMAT_RGBA8888;
				return;
			case AV_PIX_FMT_ABGR:
				sdl_pix_fmt = SDL_PIXELFORMAT_ABGR8888;
				return;
			case AV_PIX_FMT_BGRA:
				sdl_pix_fmt = SDL_PIXELFORMAT_BGRA8888;
				return;
			default:
				return;
		}
	}

	bool recreate_texture(SDL_Renderer* renderer, SDL_Texture*& texture, Uint32 format, int width, int height, SDL_BlendMode blend_mode)
	{
		bool recreate = texture == nullptr;
		if (!recreate)
		{
			Uint32 current_format{};
			int current_width{};
			int current_height{};
			SDL_QueryTexture(texture, &current_format, nullptr, &current_width, &current_height);
			recreate = current_format != format || current_width != width || current_height != height;
		}

		if (recreate)
		{
			if (texture)
				SDL_DestroyTexture(texture);

			texture = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_STREAMING, width, height);
			if (!texture)
				return false;

			if (SDL_SetTextureBlendMode(texture, blend_mode) < 0)
				return false;
		}

		return true;
	}

	bool upload_texture(SDL_Renderer* renderer, SDL_Texture*& texture, Frame& frame)
	{
		Uint32 sdl_pix_fmt{};
		SDL_BlendMode sdl_blendmode{};
		get_sdl_pix_fmt_and_blendmode(frame.frame->format, sdl_pix_fmt, sdl_blendmode);
		if (sdl_pix_fmt == SDL_PIXELFORMAT_UNKNOWN)
			return false;

		if (!recreate_texture(renderer, texture, sdl_pix_fmt, frame.frame->width, frame.frame->height, sdl_blendmode))
			return false;

		int ret = 0;
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
}

void VideoState::Open()
{
	m_readThread.Run();
}

void VideoState::EventLoop(SDL_Window* window, SDL_Renderer* renderer)
{
	m_window = window;
	m_renderer = renderer;

	SDL_Event event;
	while (true)
	{
		refresh_loop_wait_event(event);
		switch (event.type)
		{
			case SDL_MOUSEBUTTONDOWN: onMouseButtonDown(event); break;
			case SDL_MOUSEMOTION:	onMouseMotion(event); break;
			case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_EXPOSED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
					force_refresh = true;
				break;
			case SDL_QUIT: [[fallthrough]];
			case FF_QUIT_EVENT: doExit(); break;
			default: break;
		}
	}
}

SyncType VideoState::getMasterSyncType() const
{
	if (global_options.sync_type == SyncType::VideoMaster)
	{
		if (m_mediaState.video.index != -1)
			return SyncType::VideoMaster;
		else
			return SyncType::AudioMaster;
	}
	else if (global_options.sync_type == SyncType::AudioMaster)
	{
		if (m_mediaState.audio.index != -1)
			return SyncType::AudioMaster;
		else
			return SyncType::External;
	}
	else
		return SyncType::External;
}

void VideoState::refresh_loop_wait_event(SDL_Event& event)
{
	double remaining_time = 0.0;
	SDL_PumpEvents();
	while (!SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT)) 
	{
		if (!cursor_hidden && av_gettime_relative() - cursor_last_shown > CURSOR_HIDE_DELAY) 
		{
			SDL_ShowCursor(0);
			cursor_hidden = true;
		}
		if (remaining_time > 0.0)
			av_usleep((int64_t)(remaining_time * 1000000.0));
		remaining_time = REFRESH_RATE;
		video_refresh(remaining_time);
		SDL_PumpEvents();
	}
}

double VideoState::getMasterClock() const
{
	switch (getMasterSyncType())
	{
		case SyncType::VideoMaster: return m_videoClock.Get();
		case SyncType::AudioMaster: return m_audioClock.Get();
		default: return m_externalClock.Get();
	}
}

void VideoState::onMouseMotion(SDL_Event const& event, bool isMouseButtonDown)
{
	if (cursor_hidden)
	{
		SDL_ShowCursor(1);
		cursor_hidden = false;
	}
	cursor_last_shown = av_gettime_relative();

	//get mouse position if needed
	double mouseX{};
	if (isMouseButtonDown)
	{
		if (event.button.button != SDL_BUTTON_RIGHT)
			return;
		mouseX = event.button.x;
	}
	else
	{
		if (!(event.motion.state & SDL_BUTTON_RMASK))
			return;
		mouseX = event.motion.x;
	}
	seekToMousePosition(mouseX);
}

void VideoState::onMouseButtonDown(SDL_Event const& event)
{
	if (global_options.exit_on_mouse_down)
	{
		doExit();
		return;
	}

	if (event.button.button == SDL_BUTTON_LEFT)
	{
		static int64_t last_mouse_left_click = 0;
		if (av_gettime_relative() - last_mouse_left_click <= DoubleClickIntervalThreshold)
		{
			//go double screen
			toggleFullScreen();
			force_refresh = true;
			last_mouse_left_click = 0;
		}
		else
			last_mouse_left_click = av_gettime_relative();
	}

	onMouseMotion(event);
}

void VideoState::toggleFullScreen()
{
	global_options.is_full_screen = !global_options.is_full_screen;
	SDL_SetWindowFullscreen(m_window, global_options.is_full_screen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

void VideoState::seekToMousePosition(double mouseX)
{
	int windowWidth = global_options.screen_width;
	SDL_GetWindowSize(m_window, &windowWidth, nullptr);
	double const percent = std::clamp(mouseX / std::max(windowWidth, 1), 0.0, 1.0);
	m_readThread.SeekToPercent(percent);
}

void VideoState::video_refresh(double& remaining_time)
{
	if (!m_mediaState.video.stream)
		return;

	while (m_mediaState.pictq.available() > 0)
	{
		auto& lastvp = m_mediaState.pictq.peek_last();
		auto& vp = m_mediaState.pictq.peek();

		//discard stale frames
		if (vp.serial != m_mediaState.video.queue.serial)
		{
			m_mediaState.pictq.next();
			continue;
		}

		//reset frame_timer is seeked
		if (lastvp.serial != vp.serial)
			frame_timer = av_gettime_relative() / Clock::AVClockBase;

		//paused?
		if (m_mediaState.paused)
			break;

		auto last_duration = calculateDuration(lastvp, vp);
		auto const delay = calculateTargetDelay(last_duration);
		auto const now = av_gettime_relative() / Clock::AVClockBase;
		if (auto scheduledTimeDiff = frame_timer + delay - now; scheduledTimeDiff > 0)
		{
			remaining_time = std::min(scheduledTimeDiff, remaining_time);
			break;
		}

		frame_timer += delay;
		clampTimer(now);

		{
			std::lock_guard lock{ m_mediaState.pictq.mutex_ };
			if (!std::isnan(vp.pts))
				update_video_pts(vp.pts, vp.serial);
		}

		if (shouldDropFrame(vp, now))
		{
			m_mediaState.pictq.next();
			continue;
		}

		m_mediaState.pictq.next();
		force_refresh = true;
		break;
	}
	if (!global_options.display_disable && force_refresh && m_mediaState.pictq.has_shown_frame())
		display();

	force_refresh = false;
}

void VideoState::display()
{
	if (!m_renderer)
		return;

	SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
	SDL_RenderClear(m_renderer);

	if (m_mediaState.video.stream && m_mediaState.pictq.has_shown_frame())
	{
		auto& vp = m_mediaState.pictq.peek_last();
		SDL_Rect rect{};
		int width{};
		int height{};
		if (SDL_GetRendererOutputSize(m_renderer, &width, &height) < 0 || width <= 0 || height <= 0)
			SDL_GetWindowSize(m_window, &width, &height);

		calculate_display_rect(rect, 0, 0, width, height, vp);
		if (!vp.uploaded)
			upload_texture(m_renderer, m_videoTexture, vp);

		if (vp.uploaded && m_videoTexture)
		{
			SDL_RenderCopyEx(
				m_renderer,
				m_videoTexture,
				nullptr,
				&rect,
				0.0,
				nullptr,
				vp.flip_v() ? SDL_FLIP_VERTICAL : SDL_FLIP_NONE
			);
		}
	}

	SDL_RenderPresent(m_renderer);
}

void VideoState::clampTimer(double now)
{
	if (now - frame_timer > AV_SYNC_THRESHOLD_MAX)
		frame_timer = now;
}

void VideoState::update_video_pts(double pts, int serial)
{
	m_videoClock.Set(pts, serial);
}

double VideoState::calculateDuration(Frame const& vp, Frame const& nextvp) const
{
	if (vp.serial != nextvp.serial)
		return 0;

	auto const duration = nextvp.pts - vp.pts;
	if (std::isnan(duration) || duration <= 0 || duration > m_readThread.max_frame_duration)
		return vp.duration;

	return duration;
}

double VideoState::calculateTargetDelay(double last_duration) const
{
	if (getMasterSyncType() == SyncType::VideoMaster)
		return last_duration;

	auto videoClockDiff = m_videoClock.Get() - getMasterClock(); //positive means video AHEAD, negative means video lagging BEHIND
	if (!std::isnan(videoClockDiff) && std::fabs(videoClockDiff) < m_readThread.max_frame_duration)
	{
		auto const sync_threshold = std::clamp(last_duration, AV_SYNC_THRESHOLD_MIN, AV_SYNC_THRESHOLD_MAX);
		if (videoClockDiff <= -sync_threshold)
			last_duration = std::max(0.0, last_duration + videoClockDiff);
		else if (videoClockDiff >= sync_threshold)
		{
			if (last_duration > AV_SYNC_FRAMEDUP_THRESHOLD)
				last_duration += videoClockDiff;
			else
				last_duration *= 2;
		}
	}

	return last_duration;
}

bool VideoState::shouldDropFrame(Frame const& vp, double now)
{
	if (!global_options.framedrop || getMasterSyncType() == SyncType::VideoMaster)
		return false;

	if (m_mediaState.pictq.available() <= 1)
		return false;

	auto& next = m_mediaState.pictq.peek_next();
	auto const next_deadline = frame_timer + calculateDuration(vp, next);
	return now > next_deadline;
}

void VideoState::doExit()
{
	m_mediaState.abort = true;
	if (m_videoTexture)
	{
		SDL_DestroyTexture(m_videoTexture);
		m_videoTexture = nullptr;
	}
	std::exit(0);
}
