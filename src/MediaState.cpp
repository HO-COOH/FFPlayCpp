module;
#include <utility>
extern "C" {
#include <libavformat/avformat.h>
}
module ffplay;
import :MediaState;

bool MediaState::RoutePacket(AV::Packet&& packet)
{
	auto const stream_index = packet->stream_index;
	if (stream_index == audio.index)
		audio.queue.put(std::move(packet));
	else if (stream_index == video.index && !(video.stream->disposition & AV_DISPOSITION_ATTACHED_PIC))
		video.queue.put(std::move(packet));
	else if (stream_index == subtitle.index)
		subtitle.queue.put(std::move(packet));
	else return false;
	return true;
}