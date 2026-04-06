module;
#include <SDL2/SDL_events.h>
extern "C" {
#include <libavutil/time.h>
}
module ffplay;
import :VideoState;
import :Options;

void VideoState::Open()
{
	m_readThread.Run();
}

void VideoState::EventLoop(SDL_Window* window)
{
	m_window = window;

	SDL_Event event;
	while (true)
	{
		switch (event.type)
		{
			case SDL_MOUSEBUTTONDOWN: onMouseButtonDown(event); break;
			case SDL_MOUSEMOTION:	onMouseMotion(event); break;
			case SDL_QUIT: [[fall_through]];
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
	double const percent = mouseX / global_options.screen_width;
	m_readThread.SeekToPercent(percent);
}

void VideoState::video_refresh(double& remaining_time)
{
}

void VideoState::doExit()
{
}
