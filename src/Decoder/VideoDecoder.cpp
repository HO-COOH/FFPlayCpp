module;
#include <utility>
#include <expected>
extern "C" {
	#include <libavformat/avformat.h>
}
module Decoder.VideoDecoder;

std::expected<AV::Frame, int> VideoDecoder::getFrame()
{
	return std::expected<AV::Frame, int>();
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
