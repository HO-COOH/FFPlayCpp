module;
#include <utility>
#include <thread>
#include <chrono>
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
	return avformat_open_input(
		&inContextPtr,
		"https://test-videos.co.uk/vids/bigbuckbunny/mp4/h264/1080/Big_Buck_Bunny_1080_10s_1MB.mp4",
		nullptr,
		nullptr
	);
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
		avcodec_parameters_to_context(codecContext.get(), m_inContext->streams[streamIndex]->codecpar);
		codecContext->codec_id = avcodec_find_decoder(codecContext->codec_id)->id;
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
	AVPacket* packet = av_packet_alloc();
	while (true)
	{
		int ret = av_read_frame(m_inContext.get(), packet);
		m_mediaState.RoutePacket(AV::Packet{ packet });
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
