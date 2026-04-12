module;
#include <memory>
#include <type_traits>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

export module AV.RAII;

export namespace AV
{
	template <auto DeleteFunction>
	struct FFmpegDeleter {
		template <typename T>
		void operator()(T* ptr) const noexcept 
		{
			if constexpr (std::is_invocable_v<decltype(DeleteFunction), T**>) 
				DeleteFunction(&ptr);
			else
				DeleteFunction(ptr);
		}
	};

	using FomatContext = std::unique_ptr<AVFormatContext, FFmpegDeleter<avformat_close_input>>;
	using CodecContext = std::unique_ptr<AVCodecContext, FFmpegDeleter<avcodec_free_context>>;

	using Packet = std::unique_ptr<AVPacket, FFmpegDeleter<av_packet_free>>;
	using Frame = std::unique_ptr<AVFrame, FFmpegDeleter<av_frame_free>>;

	Packet packet_alloc()
	{
		return Packet{ av_packet_alloc() };
	}

	Frame frame_alloc()
	{
		return Frame{ av_frame_alloc() };
	}

	namespace Codec
	{
		CodecContext alloc_context3()
		{
			return CodecContext{ avcodec_alloc_context3(nullptr) };
		}
	}
}