module;
extern "C" {
#include <libavformat/avformat.h>
}
module ffplay;
import :ReadThread;

int ReadThread::open_input()
{
	auto inContextPtr = m_inContext.get();
	int ret = avformat_open_input(
		&inContextPtr,
		"https://test-videos.co.uk/vids/bigbuckbunny/mp4/h264/1080/Big_Buck_Bunny_1080_10s_1MB.mp4",
		nullptr,
		nullptr
	);

	streamIndex[AVMediaType::AVMEDIA_TYPE_VIDEO] = av_find_best_stream(
		m_inContext.get(),
		AVMediaType::AVMEDIA_TYPE_VIDEO,
		-1,
		-1,
		nullptr,
		0
	);
	streamIndex[AVMediaType::AVMEDIA_TYPE_AUDIO] = av_find_best_stream(
		m_inContext.get(),
		AVMediaType::AVMEDIA_TYPE_AUDIO,
		-1,
		-1,
		nullptr,
		0
	);
	streamIndex[AVMediaType::AVMEDIA_TYPE_SUBTITLE] = av_find_best_stream(
		m_inContext.get(),
		AVMediaType::AVMEDIA_TYPE_SUBTITLE,
		-1,
		-1,
		nullptr,
		0
	);
}

int ReadThread::set_streams()
{
	return 0;
}

int ReadThread::open_decoders()
{
	return 0;
}

void ReadThread::read_loop()
{
}

void ReadThread::Run()
{
}
