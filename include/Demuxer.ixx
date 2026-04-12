module;
#include <condition_variable>
extern "C" {
#include <libavformat/avformat.h>
}
export module ffplay:Demuxer;

import AV.RAII;
import :MediaState;
import :SeekRequest;
import :Options;

export class Demuxer
{
	MediaState& m_mediaState;
	AV::FomatContext m_inContext{ avformat_alloc_context() };
	AV::Packet m_packet = AV::packet_alloc();
	AVStream* m_stream{};
	SeekRequest m_seekRequest;
	std::condition_variable m_continue_read_thread;
	int streamIndexMap[AVMediaType::AVMEDIA_TYPE_NB]{ -1, -1, -1, -1, -1 };

	std::jthread m_thread;

	int open_input(char const* url);
	int set_streams(Options const& options);
	int open_decoders();
	void read_loop();
public:
	double max_frame_duration{};
	Demuxer(MediaState& mediaState);
	int Open(char const* url, Options const& options);
	void SeekToPercent(double percent, bool seek_by_bytes);
};