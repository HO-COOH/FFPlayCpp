export module ffplay:MediaState;

import :StreamSlot;
import AV.RAII;

export class MediaState
{
	StreamSlot audio;
	StreamSlot video;
	StreamSlot subtitle;

public:
	bool RoutePacket(AV::Packet&& packet);
};
