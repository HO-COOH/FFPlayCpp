module;
#include <condition_variable>
extern "C" {
#include <libavformat/avformat.h>
}
export module ffplay:ReadThread;

import AV.RAII;
import :MediaState;
import :SeekRequest;

export class ReadThread
{
	MediaState& m_mediaState;
	AV::FomatContext m_inContext{ avformat_alloc_context() };
	AV::Packet m_packet{ av_packet_alloc() };
	AVStream* m_stream{};
	SeekRequest m_seekRequest;
	std::condition_variable m_continue_read_thread;
	int streamIndexMap[AVMediaType::AVMEDIA_TYPE_NB]{ -1, -1, -1, -1, -1 };

	std::jthread m_thread;

	int open_input();
	int set_streams();
	int open_decoders();
	void read_loop();
public:
	double max_frame_duration{};
	ReadThread(MediaState& mediaState);
	void Run();
	void SeekToPercent(double percent);
};