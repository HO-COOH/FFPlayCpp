module;
#include <utility>
#include <thread>
#include <chrono>
#include <format>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}
module ffplay;
import :ReadThread;
import :Options;
import AV.RAII;

int ReadThread::open_input()
{
	auto inContextPtr = m_inContext.get();
	int const ret = avformat_open_input(
		&inContextPtr,
		"https://test-videos.co.uk/vids/bigbuckbunny/mp4/h264/1080/Big_Buck_Bunny_1080_10s_1MB.mp4",
		nullptr,
		nullptr
	);
	if (ret < 0)
		return ret;

	if (int const streamInfoRet = avformat_find_stream_info(m_inContext.get(), nullptr); streamInfoRet < 0)
		return streamInfoRet;

	max_frame_duration = (m_inContext->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;
	return 0;
}

int ReadThread::set_streams()
{
	if (!global_options.disable_video)
	{
		streamIndexMap[AVMediaType::AVMEDIA_TYPE_VIDEO] = av_find_best_stream(
			m_inContext.get(),
			AVMediaType::AVMEDIA_TYPE_VIDEO,
			-1,
			-1,
			nullptr,
			0
		);
	}

	if (!global_options.disable_audio)
	{
		streamIndexMap[AVMediaType::AVMEDIA_TYPE_AUDIO] = av_find_best_stream(
			m_inContext.get(),
			AVMediaType::AVMEDIA_TYPE_AUDIO,
			-1,
			-1,
			nullptr,
			0
		);
	}

	if (!global_options.disable_video && !global_options.disable_subtitle)
	{
		streamIndexMap[AVMediaType::AVMEDIA_TYPE_SUBTITLE] = av_find_best_stream(
			m_inContext.get(),
			AVMediaType::AVMEDIA_TYPE_SUBTITLE,
			-1,
			-1,
			nullptr,
			0
		);
	}

	return 1;
}

int ReadThread::open_decoders()
{
	if (auto const streamIndex = streamIndexMap[AVMediaType::AVMEDIA_TYPE_VIDEO]; streamIndex >= 0)
	{
		m_mediaState.video.index = streamIndex;
		m_mediaState.video.stream = m_inContext->streams[streamIndex];
		AV::CodecContext codecContext{ avcodec_alloc_context3(nullptr) };
		if (!codecContext)
			return AVERROR(ENOMEM);

		int const parametersToContextRet = avcodec_parameters_to_context(codecContext.get(), m_inContext->streams[streamIndex]->codecpar);
		if (parametersToContextRet < 0)
			return parametersToContextRet;

		auto* codec = avcodec_find_decoder(codecContext->codec_id);
		if (!codec)
			return AVERROR_DECODER_NOT_FOUND;

		codecContext->pkt_timebase = m_inContext->streams[streamIndex]->time_base;
		int const openCodecRet = avcodec_open2(codecContext.get(), codec, nullptr);
		if (openCodecRet < 0)
			return openCodecRet;

		m_mediaState.video.queue.start();
		m_mediaState.videoDecoder.emplace(
			m_inContext.get(),
			std::move(codecContext),
			m_mediaState.video,
			m_continue_read_thread
		);
		m_mediaState.videoDecoder->Start(m_mediaState.pictq);
	}
	return 1;
}

void ReadThread::read_loop()
{
	while (!m_mediaState.abort)
	{
		AV::Packet packet{ av_packet_alloc() };
		if (!packet)
			return;

		int const ret = av_read_frame(m_inContext.get(), packet.get());
		if (ret < 0)
			return;

		m_mediaState.RoutePacket(std::move(packet));
	}
}

ReadThread::ReadThread(MediaState& mediaState) : m_mediaState{mediaState}
{
}

void ReadThread::Run()
{
	open_input();
	set_streams();
	open_decoders();

	m_thread = std::jthread{ &ReadThread::read_loop, this };
}

void ReadThread::SeekToPercent(double percent)
{
	if (m_seekRequest.index() != 0)
		return;

	if (global_options.seek_by_bytes || m_inContext->duration <= 0)
	{
		auto const totalBytes = avio_size(m_inContext->pb);
		m_seekRequest = SeekByBytes{ static_cast<int64_t>(percent * totalBytes) };
	}
	else
	{

		//seek by microseconds
		std::chrono::microseconds duration{ m_inContext->duration };
		auto const target = duration * percent;

		//log
		av_log(
			NULL,
			AV_LOG_INFO,
			"%s",
			std::format(
				"Seek to {:.0f}% ({:%T}) of total duration ({:%T})       \n",
				percent * 100.0,
				std::chrono::duration_cast<std::chrono::seconds>(target),
				std::chrono::duration_cast<std::chrono::seconds>(duration)
			).data()
		);

		auto microsecondCount = target.count();
		if (m_inContext->start_time != AV_NOPTS_VALUE)
			microsecondCount += m_inContext->start_time;

		m_seekRequest = SeekByTime{ static_cast<int64_t>(microsecondCount) };
	}
	m_continue_read_thread.notify_all();
}
