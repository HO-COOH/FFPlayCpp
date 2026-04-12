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
import :Player;
import :Options;
import utils.Clock;

Player::Player(Options options) : 
	m_options{std::move(options)},
	m_window{ m_options.program_name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, options.screen_width, options.screen_height, 0},
	m_renderer{m_window.Get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC}
{
}

int Player::Open(char const* url)
{
	return m_readThread.Open(url, m_options);
}

void Player::EventLoop()
{

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

SyncType Player::getMasterSyncType() const
{
	if (m_options.sync_type == SyncType::VideoMaster)
	{
		if (m_mediaState.video.index != -1)
			return SyncType::VideoMaster;
		else
			return SyncType::AudioMaster;
	}
	else if (m_options.sync_type == SyncType::AudioMaster)
	{
		if (m_mediaState.audio.index != -1)
			return SyncType::AudioMaster;
		else
			return SyncType::External;
	}
	else
		return SyncType::External;
}

void Player::refresh_loop_wait_event(SDL_Event& event)
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

double Player::getMasterClock() const
{
	switch (getMasterSyncType())
	{
		case SyncType::VideoMaster: return m_videoClock.Get();
		case SyncType::AudioMaster: return m_audioClock.Get();
		default: return m_externalClock.Get();
	}
}

void Player::onMouseMotion(SDL_Event const& event, bool isMouseButtonDown)
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

void Player::onMouseButtonDown(SDL_Event const& event)
{
	if (m_options.exit_on_mouse_down)
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

void Player::toggleFullScreen()
{
	m_options.is_full_screen = !m_options.is_full_screen;
	m_window.SetFullscreen(m_options.is_full_screen);
}

void Player::seekToMousePosition(double mouseX)
{
	auto [windowWidth, _] = m_window.GetSize();
	double const percent = std::clamp(mouseX / std::max(windowWidth, 1), 0.0, 1.0);
	m_readThread.SeekToPercent(percent, m_options.seek_by_bytes);
}

void Player::video_refresh(double& remaining_time)
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
	if (!m_options.display_disable && force_refresh && m_mediaState.pictq.has_shown_frame())
		display();

	force_refresh = false;
}

void Player::display()
{
	m_renderer.Clear();

	if (m_mediaState.video.stream && m_mediaState.pictq.has_shown_frame())
	{
		auto& vp = m_mediaState.pictq.peek_last();
		m_renderer.Display(m_window.Get(), vp);
	}

	m_renderer.Present();
}

void Player::clampTimer(double now)
{
	if (now - frame_timer > AV_SYNC_THRESHOLD_MAX)
		frame_timer = now;
}

void Player::update_video_pts(double pts, int serial)
{
	m_videoClock.Set(pts, serial);
}

double Player::calculateDuration(Frame const& vp, Frame const& nextvp) const
{
	if (vp.serial != nextvp.serial)
		return 0;

	auto const duration = nextvp.pts - vp.pts;
	if (std::isnan(duration) || duration <= 0 || duration > m_readThread.max_frame_duration)
		return vp.duration;

	return duration;
}

double Player::calculateTargetDelay(double last_duration) const
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

bool Player::shouldDropFrame(Frame const& vp, double now)
{
	if (!m_options.framedrop || getMasterSyncType() == SyncType::VideoMaster)
		return false;

	if (m_mediaState.pictq.available() <= 1)
		return false;

	auto& next = m_mediaState.pictq.peek_next();
	auto const next_deadline = frame_timer + calculateDuration(vp, next);
	return now > next_deadline;
}

void Player::doExit()
{
	m_mediaState.abort = true;
	std::exit(0);
}
