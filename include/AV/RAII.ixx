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

	using FomatContext = std::unique_ptr<AVFormatContext, FFmpegDeleter<avformat_free_context>>;
	using Packet = std::unique_ptr<AVPacket, FFmpegDeleter<av_packet_free>>;
	using CodecContext = std::unique_ptr<AVCodecContext, FFmpegDeleter<avcodec_free_context>>;
	using Frame = std::unique_ptr<AVFrame, FFmpegDeleter<av_frame_free>>;
}