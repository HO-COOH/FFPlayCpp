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
import :Demuxer;
import :Options;
import AV.RAII;

int Demuxer::open_input(char const* url)
{
	auto inContextPtr = m_inContext.release();
	int ret = avformat_open_input(
		&inContextPtr,
		url,
		nullptr,
		nullptr
	);
	m_inContext.reset(inContextPtr);
	if (ret < 0)
		return ret;

	if (ret = avformat_find_stream_info(m_inContext.get(), nullptr); ret < 0)
		return ret;

	max_frame_duration = (m_inContext->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;
	return 0;
}

int Demuxer::set_streams(Options const& options)
{
	if (!options.disable_video)
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

	if (!options.disable_audio)
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

	if (!options.disable_video && !options.disable_subtitle)
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

int Demuxer::open_decoders()
{
	auto const streamIndex = streamIndexMap[AVMediaType::AVMEDIA_TYPE_VIDEO];
	if (streamIndex < 0)
		return -1;

	m_mediaState.video.index = streamIndex;
	m_mediaState.video.stream = m_inContext->streams[streamIndex];
	auto codecContext = AV::Codec::alloc_context3();

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
	return 0;

}

void Demuxer::read_loop()
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

Demuxer::Demuxer(MediaState& mediaState) : m_mediaState{mediaState}
{
}

int Demuxer::Open(char const* url, Options const& options)
{
	int ret = open_input(url);
	if (ret < 0)
		return ret;

	ret = set_streams(options);
	if (ret < 0)
		return ret;

	ret = open_decoders();
	if (ret < 0)
		return ret;

	m_thread = std::jthread{ &Demuxer::read_loop, this };
	return 0;
}

void Demuxer::SeekToPercent(double percent, bool seek_by_bytes)
{
	if (m_seekRequest.index() != 0)
		return;

	if (seek_by_bytes || m_inContext->duration <= 0)
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
