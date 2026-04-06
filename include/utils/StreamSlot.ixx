module;
extern "C" {
#include <libavformat/avformat.h>
}
export module utils.StreamSlot;

import utils.PacketQueue;

export struct StreamSlot
{
	int index = -1;
	AVStream* stream{};
	PacketQueue queue;
};