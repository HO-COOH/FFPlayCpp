module;
#include <mutex>
#include <condition_variable>
extern "C" {
#include <libavcodec/packet.h>
}
module utils.PacketQueue;

int PacketQueue::putImpl(AVPacket* pkt)
{
	if (auto const ret = m_pkt_list.push(MyAVPacketInternal{ pkt, serial }); ret < 0)
		return ret;

	++nb_packets;
	size += (pkt->size + sizeof(MyAVPacketInternal));
	duration += pkt->duration;
	m_cond.notify_all();
	return 0;
}

void PacketQueue::flush()
{
	std::lock_guard lock{ m_mutex };
	MyAVPacketInternal pkt;
	while (m_pkt_list.try_pop(pkt))
		av_packet_free(&pkt.packet);
	nb_packets = 0;
	size = 0;
	duration = 0;
	++serial;
}

void PacketQueue::start()
{
	std::lock_guard lock{ m_mutex };
	abort_request = false;
	++serial;
}

void PacketQueue::abort()
{
	std::lock_guard lock{ m_mutex };
	abort_request = true;
	m_cond.notify_all();
}

std::expected<MyAVPacket, int> PacketQueue::get(bool block)
{
	std::unique_lock lock{ m_mutex };
	while (true)
	{
		if (abort_request)
			return std::unexpected{ QueueError::Abort };

		MyAVPacketInternal pkt;
		if (m_pkt_list.try_pop(pkt))
		{
			--nb_packets;
			size -= pkt.packet->size + sizeof(pkt);
			duration -= pkt.packet->duration;
			return MyAVPacket{ AV::Packet{pkt.packet}, pkt.serial };
		}

		if (!block)
			return std::unexpected{ QueueError::Empty };

		m_cond.wait(lock);
	}
}

int PacketQueue::put(AV::Packet&& packet)
{
	std::lock_guard lock{ m_mutex };
	if (auto raw = packet.release())
	{
		if (auto const ret = putImpl(raw); ret < 0)
		{
			av_packet_free(&raw);
			return ret;
		}
	}
	return 0;
}

PacketQueue::~PacketQueue()
{
	flush();
}
