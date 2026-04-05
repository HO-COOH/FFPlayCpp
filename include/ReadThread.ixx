module;
extern "C" {
#include <libavformat/avformat.h>
}
export module ffplay:ReadThread;

import AV.RAII;

export class ReadThread
{
	AV::FomatContext m_inContext{ avformat_alloc_context() };
	AV::Packet m_packet{ av_packet_alloc() };
	AVStream* m_stream{};

	int streamIndex[AVMediaType::AVMEDIA_TYPE_NB]{};

	int open_input();
	int set_streams();
	int open_decoders();
	void read_loop();
public:
	void Run();
};