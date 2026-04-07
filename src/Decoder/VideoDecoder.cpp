module;
#include <utility>
#include <expected>
#include <cmath>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}
module Decoder.VideoDecoder;

std::expected<AV::Frame, int> VideoDecoder::getFrame()
{
	AV::Frame frame{ av_frame_alloc() };
	if (!frame)
		return std::unexpected{ AVERROR(ENOMEM) };

	while (true)
	{
		int ret = avcodec_receive_frame(m_context.get(), frame.get());
		if (ret >= 0)
		{
			return std::move(frame);
		}

		if (ret == AVERROR_EOF)
			return std::unexpected{ ret };

		if (ret != AVERROR(EAGAIN))
			return std::unexpected{ ret };

		auto packet = stream_slot_ref.queue.get(true);
		if (!packet.has_value())
			return std::unexpected{ packet.error() };

		m_packetSerial = packet->serial;
		ret = avcodec_send_packet(m_context.get(), packet->packet.get());
		if (ret == AVERROR(EAGAIN))
			continue;
		if (ret < 0)
			return std::unexpected{ ret };
	}
}

void VideoDecoder::reallocFilterIfNeeded(AV::Frame const& frame)
{
}

VideoDecoder::VideoDecoder(
	AVFormatContext* formatContext,
	AV::CodecContext&& codecContext,
	StreamSlot& stream_slot,
	std::condition_variable& empty_queue_cond) : 
	m_context{ std::move(codecContext) }, 
	stream_slot_ref{ stream_slot }, 
	empty_queue_cond_ref{ empty_queue_cond },
	m_frameRate{av_guess_frame_rate(formatContext, stream_slot.stream, nullptr) }
{
}
