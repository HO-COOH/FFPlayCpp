module;
#include <mutex>
#include <expected>
extern "C" {
#include <libavcodec/packet.h>
}

export module utils.PacketQueue;

import AV.Fifo;
import AV.RAII;

struct MyAVPacketInternal
{
	AVPacket* packet{};
	int serial{};
};

export struct MyAVPacket
{
	AV::Packet packet;
	int serial{};
};

export class PacketQueue
{
	AV::Fifo<MyAVPacketInternal> m_pkt_list{ 1 };
	int nb_packets{};
	int size{};
	int64_t duration{};

	bool abort_request{ true };
	std::mutex m_mutex;
	std::condition_variable m_cond;

	int putImpl(AVPacket* pkt);
public:
	int serial{};
	void flush();
	void start();
	void abort();

	std::expected<MyAVPacket, int> get(bool block);
	int put(AV::Packet&& packet);

	~PacketQueue();
};