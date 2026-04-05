module;
#include <cstdint>
#include <string>

export module ffplay:Options;

export struct Options
{
	int32_t width{};
	int32_t height{};
	bool is_full_screen{};
	bool disable_audio{};
	bool disable_video{};
	bool disable_subtitle{};
	std::string desired_audio_stream;
	std::string desired_video_stream;
	std::string desired_subtitle_stream;
	float start_time{};
	float duration{};
	bool seek_by_bytes{};
	float seek_interval{};
	bool display_disable{};
	bool always_on_top{};
	int32_t volume{};
	std::string force_format;
	bool show_status{};
	bool fast{};
	bool generate_pts{};
	bool decoder_reorder_pts{};
	int32_t lowres{};
	int32_t sync_type{};
	bool auto_exit{};
	bool exit_on_key_down{};
	bool exit_on_mouse_down{};
	int32_t loop{};
	bool framedrop{};
	bool infinite_buffer{};
	std::string window_title;
	int32_t screen_left{};
	int32_t screen_top{};
	std::string video_filter;
	std::string audio_filter;
	std::string audio_codec;
	std::string video_codec;
	std::string subtitle_codec;
	bool auto_rotate{};
	bool find_stream_info{};
	int32_t filter_threads{};
	std::string video_background;
	std::string hardware_accelerate;
};