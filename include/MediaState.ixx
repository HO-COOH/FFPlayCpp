module;
#include <optional>
export module ffplay:MediaState;

import utils.StreamSlot;
import AV.RAII;
import Decoder.VideoDecoder;
import utils.FrameQueue;

export class MediaState
{
	constexpr static auto VIDEO_PICTURE_QUEUE_SIZE = 3;
	constexpr static auto SUBPICTURE_QUEUE_SIZE = 16;
	constexpr static auto SAMPLE_QUEUE_SIZE = 9;

public:
	StreamSlot audio;
	StreamSlot video;
	StreamSlot subtitle;
	std::optional<VideoDecoder> videoDecoder;


	FrameQueue<VIDEO_PICTURE_QUEUE_SIZE> pictq{ video.queue, true };
	FrameQueue<SUBPICTURE_QUEUE_SIZE> subpq{ subtitle.queue, false };
	FrameQueue<SAMPLE_QUEUE_SIZE> sampq{ audio.queue, true };

	bool abort{};
	bool paused{};

	bool RoutePacket(AV::Packet&& packet);
};
