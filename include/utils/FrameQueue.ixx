module;
#include <cstddef>
extern "C" {
#include <libavutil/frame.h>
}
export module utils.FrameQueue;

import AV.RAII;
import utils.PacketQueue;
import utils.RingBuffer;

export template<std::size_t Capacity>
class FrameQueue : public RingBuffer<AV::Frame, Capacity>
{
	PacketQueue const& packetQueue_;
public:
	explicit FrameQueue(PacketQueue const& packetQueue, bool keep_last = true) : RingBuffer<AV::Frame, Capacity>(keep_last), packetQueue_(packetQueue)
	{
		for (std::size_t i = 0; i < Capacity; i++)
			this->queue_[i].reset(av_frame_alloc());
	}
};