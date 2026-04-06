module;
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_video.h>
export module ffplay:VideoState;

import :SyncType;
import :ReadThread;
import utils.Clock;
import utils.PacketQueue;
import utils.FrameQueue;
import :MediaState;

export class VideoState
{
public:
	void Open();

	void EventLoop(SDL_Window* window);
private:
	SDL_Window* m_window{ nullptr };
	Clock m_audioClock;
	Clock m_videoClock;
	Clock m_externalClock;

	MediaState m_mediaState;
	ReadThread m_readThread{ m_mediaState };
	int64_t cursor_last_shown{};
	bool cursor_hidden{};
	bool force_refresh{};

	SyncType getMasterSyncType() const;
	//{

	//}
	void refresh_loop_wait_event(SDL_Event& event);
	double getMasterClock() const;

	void onMouseMotion(SDL_Event const& event, bool isMouseButtonDown = false);
	void onMouseButtonDown(SDL_Event const& event);
	void toggleFullScreen();
	void seekToMousePosition(double mouseX);
	void video_refresh(double& remaining_time);

	void doExit();
	constexpr static auto CURSOR_HIDE_DELAY = 1000000;
	constexpr static auto REFRESH_RATE = 0.01;
	constexpr static int64_t DoubleClickIntervalThreshold = 500000;
	constexpr static auto FF_QUIT_EVENT = (SDL_USEREVENT + 2);
};