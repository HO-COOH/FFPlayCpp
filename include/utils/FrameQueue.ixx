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
	mutable std::mutex mutex_;
	std::condition_variable cond_;
	std::atomic<bool> abort_{ false };

	int rindex_shown_ = 0;
	bool keep_last_;

	constexpr int read_index() const noexcept {
		return this->wrap(this->rindex_ + rindex_shown_);
	}

public:
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

	void push() {
		this->windex_ = this->wrap(this->windex_ + 1);
		{
			std::lock_guard<std::mutex> lock(mutex_);
			this->size_++;
		}
		cond_.notify_one();
	}


	template<typename CleanupFn = decltype([](AV::Frame&) {}) >
	void next(CleanupFn cleanup = {}) 
	{
		if (keep_last_ && !rindex_shown_)
		{
			rindex_shown_ = 1;
			return;
		}

		cleanup(this->queue_[this->rindex_]);

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
			cleanup(this->queue_[this->rindex_]);
			this->rindex_ = this->wrap(this->rindex_ + 1);
			this->size_--;
		}
		this->windex_ = 0;
		this->rindex_ = 0;
		rindex_shown_ = 0;
		cond_.notify_one();
	}
};
