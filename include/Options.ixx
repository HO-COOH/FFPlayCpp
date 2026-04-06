module;
#include <cstdint>
#include <string>

export module ffplay:Options;
import :SyncType;

export struct Options
{
	int32_t screen_width = 640;
	int32_t screen_height = 480;
	bool is_full_screen{};
	bool disable_audio{};
	bool disable_video{};
	bool disable_subtitle{};
	bool show_status{};
	bool fast{};
	bool generate_pts{};
	bool decoder_reorder_pts{};

	std::string desired_audio_stream;
	std::string desired_video_stream;
	std::string desired_subtitle_stream;
	float start_time{};
	float duration{};
	bool seek_by_bytes{};
	bool auto_exit{};
	bool exit_on_key_down{};
	bool exit_on_mouse_down{};
	float seek_interval{};
	bool display_disable{};
	bool always_on_top{};
	bool framedrop{};
	bool infinite_buffer{};
	bool find_stream_info{};
	bool auto_rotate{};

	std::string force_format;

	int32_t lowres{};
	SyncType sync_type{ SyncType::AudioMaster };

	int32_t loop{};
	int32_t volume{};
	std::string window_title;
	int32_t screen_left{};
	int32_t screen_top{};
	std::string video_filter;
	std::string audio_filter;
	std::string audio_codec;
	std::string video_codec;
	std::string subtitle_codec;


	int32_t filter_threads{};
	std::string video_background;
	std::string hardware_accelerate;
};

// Declare the global variable so other modules can use it
export extern Options global_options;