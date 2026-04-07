module;
#include <condition_variable>
#include <thread>
#include <expected>
extern "C" {
#include <libavformat/avformat.h>
}
export module Decoder.VideoDecoder;
import AV.RAII;
import utils.PacketQueue;
import utils.StreamSlot;
import utils.FrameQueue;

export class VideoDecoder
{
	AV::CodecContext m_context;
	//PacketQueue& packet_queue_ref;
	StreamSlot& stream_slot_ref;
	std::condition_variable& empty_queue_cond_ref;
	std::jthread m_thread;
	AVRational m_frameRate;
	int m_packetSerial{};

	template<std::size_t Capacity>
	void queue_frame(FrameQueue<Capacity>& frameQueue, AV::Frame&& frame)
	{
		auto const pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(stream_slot_ref.stream->time_base);
		auto const duration = (m_frameRate.num && m_frameRate.den) ? av_q2d(AVRational{ m_frameRate.den, m_frameRate.num }) : 0;

		auto videoPicture = frameQueue.peek_writable();
		if (!videoPicture)
			return;
		videoPicture->fill(frame.get());
		videoPicture->pts = pts;
		videoPicture->duration = duration;
		videoPicture->serial = m_packetSerial;
		videoPicture->uploaded = false;
		av_frame_move_ref(videoPicture->frame, frame.get());
		frameQueue.push();
	}


	template<std::size_t Capacity>
	void threadProc(FrameQueue<Capacity>& frameQueue)
	{
		while (true)
		{
			auto got_frame = getFrame();
			if (!got_frame.has_value())
				return;

			reallocFilterIfNeeded(*got_frame);
			queue_frame(frameQueue, std::move(*got_frame));
		}
	}

	std::expected<AV::Frame, int> getFrame();
	void reallocFilterIfNeeded(AV::Frame const& frame);
public:
	VideoDecoder(
		AVFormatContext* formatContext,
		AV::CodecContext&& codecContext,
		StreamSlot& stream_slot,
		std::condition_variable& empty_queue_cond
	);

	//VideoDecoder(
	//	AVStream* stream
	//);

	template<std::size_t Capacity>
	void Start(FrameQueue<Capacity>& frameQueue)
	{
		m_thread = std::jthread{ &VideoDecoder::threadProc<Capacity>, this, std::ref(frameQueue)};
	}
};
