module;
extern "C" {
#include <libavformat/avformat.h>
}
export module ffplay:StreamSlot;

import utils.PacketQueue;

export struct StreamSlot
{
	int index = -1;
	AVStream* stream{};
	PacketQueue queue;
};