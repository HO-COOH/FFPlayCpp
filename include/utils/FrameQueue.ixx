module;
#include <cstddef>
#include <mutex>
#include <condition_variable>
#include <atomic>
extern "C" {
#include <libavutil/frame.h>
}
export module utils.FrameQueue;

import AV.RAII;
import utils.PacketQueue;
import utils.RingBuffer;
import utils.Frame;

export template<std::size_t Capacity>
class FrameQueue : public RingBuffer<Frame, Capacity>
{
	PacketQueue const& packetQueue_;

	std::condition_variable cond_;
	std::atomic<bool> abort_{ false };

	bool rindex_shown_ = 0;
	bool keep_last_;

	constexpr int read_index() const noexcept {
		return this->wrap(this->rindex_ + rindex_shown_);
	}

public:
	mutable std::mutex mutex_;
	explicit FrameQueue(PacketQueue const& packetQueue, bool keep_last = true) : packetQueue_(packetQueue), keep_last_(keep_last)
	{
		for (std::size_t i = 0; i < Capacity; i++)
			this->queue_[i].frame = av_frame_alloc();
	}

	void request_abort() noexcept {
		abort_.store(true, std::memory_order_release);
		cond_.notify_all();
	}

	void clear_abort() noexcept {
		abort_.store(false, std::memory_order_release);
	}

	[[nodiscard]] bool aborted() const noexcept {
		return abort_.load(std::memory_order_acquire);
	}

	[[nodiscard]] Frame* peek_writable() {
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.wait(lock, [this] {
			return this->size_ < static_cast<int>(Capacity)
				|| abort_.load(std::memory_order_acquire);
			});
		if (abort_.load(std::memory_order_acquire))
			return nullptr;
		return &this->queue_[this->windex_];
	}

	[[nodiscard]] Frame& peek_last()
	{
		return this->queue_[this->rindex_];
	}

	[[nodiscard]] Frame& peek()
	{
		return this->queue_[read_index()];
	}

	[[nodiscard]] Frame& peek_next()
	{
		return this->queue_[this->wrap(this->rindex_ + rindex_shown_ + 1)];
	}

	void push() {
		this->windex_ = this->wrap(this->windex_ + 1);
		{
			std::lock_guard<std::mutex> lock(mutex_);
			this->size_++;
		}
		cond_.notify_one();
	}


	void next() 
	{
		if (keep_last_ && !rindex_shown_)
		{
			rindex_shown_ = 1;
			return;
		}

		auto& frame = this->queue_[this->rindex_];
		av_frame_unref(frame.frame);
		frame.uploaded = false;

		this->rindex_ = this->wrap(this->rindex_ + 1);
		{
			std::lock_guard<std::mutex> lock(mutex_);
			this->size_--;
		}
		cond_.notify_one();
	}

	template<typename CleanupFn = decltype([](AV::Frame&) {}) >
	void flush(CleanupFn cleanup = {}) 
	{
		std::lock_guard<std::mutex> lock(mutex_);
		while (this->size_ > 0) {
			auto& frame = this->queue_[this->rindex_];
			cleanup(frame);
			av_frame_unref(frame.frame);
			frame.uploaded = false;
			this->rindex_ = this->wrap(this->rindex_ + 1);
			this->size_--;
		}
		this->windex_ = 0;
		this->rindex_ = 0;
		rindex_shown_ = 0;
		cond_.notify_one();
	}

	[[nodiscard]] int available() const noexcept
	{
		return this->size() - rindex_shown_;
	}

	[[nodiscard]] bool has_shown_frame() const noexcept
	{
		return rindex_shown_;
	}

	~FrameQueue()
	{
		for (auto& frame : this->queue_)
			av_frame_free(&frame.frame);
	}
};
