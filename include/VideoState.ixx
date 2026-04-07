module;
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_render.h>
export module ffplay:VideoState;

import :SyncType;
import :ReadThread;
import utils.Clock;
import utils.PacketQueue;
import utils.FrameQueue;
import :MediaState;
import utils.Frame;

export class VideoState
{
public:
	void Open();

	void EventLoop(SDL_Window* window, SDL_Renderer* renderer);
private:
	SDL_Window* m_window{ nullptr };
	SDL_Renderer* m_renderer{ nullptr };
	SDL_Texture* m_videoTexture{ nullptr };
	Clock m_audioClock;
	Clock m_videoClock;
	Clock m_externalClock;

	MediaState m_mediaState;
	ReadThread m_readThread{ m_mediaState };
	int64_t cursor_last_shown{};
	double frame_timer{};
	bool cursor_hidden{};
	bool force_refresh{};

	[[nodiscard]] SyncType getMasterSyncType() const;
	void refresh_loop_wait_event(SDL_Event& event);
	[[nodiscard]] double getMasterClock() const;

	void onMouseMotion(SDL_Event const& event, bool isMouseButtonDown = false);
	void onMouseButtonDown(SDL_Event const& event);
	void toggleFullScreen();
	void seekToMousePosition(double mouseX);
	void video_refresh(double& remaining_time);
	void display();
	void clampTimer(double now);
	void update_video_pts(double pts, int serial);
	[[nodiscard]] double calculateDuration(Frame const& vp, Frame const& nextvp) const;
	[[nodiscard]] double calculateTargetDelay(double last_duration) const;
	[[nodiscard]] bool shouldDropFrame(Frame const& vp, double now);
	void doExit();
	constexpr static auto CURSOR_HIDE_DELAY = 1000000;
	constexpr static auto REFRESH_RATE = 0.01;
	constexpr static int64_t DoubleClickIntervalThreshold = 500000;
	constexpr static auto FF_QUIT_EVENT = (SDL_USEREVENT + 2);

	/* no AV sync correction is done if below the minimum AV sync threshold */
	constexpr static auto AV_SYNC_THRESHOLD_MIN = 0.04;
	/* AV sync correction is done if above the maximum AV sync threshold */
	constexpr static auto AV_SYNC_THRESHOLD_MAX = 0.1;
	/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
	constexpr static auto AV_SYNC_FRAMEDUP_THRESHOLD = 0.1;
	/* no AV correction is done if too big error */
	constexpr static auto AV_NOSYNC_THRESHOLD = 10.0;
};