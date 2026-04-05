module;
#include <array>
#include <mutex>
#include <condition_variable>
#include <atomic>

export module utils.RingBuffer;

export template<typename T, std::size_t Capacity>
class RingBuffer
{
protected:
    std::array<T, Capacity> queue_{};
    int rindex_ = 0;
    int windex_ = 0;
    int size_ = 0;
    int rindex_shown_ = 0;
    bool keep_last_;

    mutable std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic<bool> abort_{ false };

    // ©¤©¤ internal helpers ©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤

    static constexpr int wrap(int index) noexcept {
        return index % static_cast<int>(Capacity);
    }

    constexpr int read_index() const noexcept {
        return wrap(rindex_ + rindex_shown_);
    }

public:
    // ©¤©¤ types ©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤

    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;

    // ©¤©¤ construction ©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤

    constexpr explicit RingBuffer(bool keep_last = true) noexcept
        : keep_last_(keep_last) {
    }

    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    // ©¤©¤ capacity ©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤

    static constexpr size_type capacity() noexcept {
        return Capacity;
    }

    [[nodiscard]] int size() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return size_;
    }

    [[nodiscard]] int remaining() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return size_ - rindex_shown_;
    }

    [[nodiscard]] bool empty() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return size_ == 0;
    }

    [[nodiscard]] bool full() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return size_ >= static_cast<int>(Capacity);
    }

    // ©¤©¤ abort control ©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤

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

    // ©¤©¤ producer interface (single writer thread) ©¤©¤©¤©¤©¤©¤©¤©¤©¤

    // Get writable slot, blocks until space available
    // Returns nullptr only on abort
    [[nodiscard]] T* peek_writable() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] {
            return size_ < static_cast<int>(Capacity)
                || abort_.load(std::memory_order_acquire);
            });
        if (abort_.load(std::memory_order_acquire))
            return nullptr;
        return &queue_[windex_];
    }

    // Commit the written slot, advance write index
    void push() {
        windex_ = wrap(windex_ + 1);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            size_++;
        }
        cond_.notify_one();
    }

    // ©¤©¤ consumer interface (single reader thread) ©¤©¤©¤©¤©¤©¤©¤©¤©¤

    // Peek at current readable frame
    [[nodiscard]] constexpr reference peek() noexcept {
        return queue_[read_index()];
    }

    [[nodiscard]] constexpr const_reference peek() const noexcept {
        return queue_[read_index()];
    }

    // Peek at next frame (one ahead of current)
    [[nodiscard]] constexpr reference peek_next() noexcept {
        return queue_[wrap(read_index() + 1)];
    }

    [[nodiscard]] constexpr const_reference peek_next() const noexcept {
        return queue_[wrap(read_index() + 1)];
    }

    // Peek at last shown frame (only valid when keep_last && rindex_shown_)
    [[nodiscard]] constexpr reference peek_last() noexcept {
        return queue_[rindex_];
    }

    [[nodiscard]] constexpr const_reference peek_last() const noexcept {
        return queue_[rindex_];
    }

    // Advance read position
    // Calls cleanup on the frame being released (if any)
    template<typename CleanupFn = decltype([](T&) {}) >
    void next(CleanupFn cleanup = {}) {
        if (keep_last_ && !rindex_shown_) {
            rindex_shown_ = 1;
            return;
        }

        cleanup(queue_[rindex_]);

        rindex_ = wrap(rindex_ + 1);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            size_--;
        }
        cond_.notify_one();
    }

    // ©¤©¤ bulk operations ©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤

    // Flush all frames, calling cleanup on each
    template<typename CleanupFn = decltype([](T&) {}) >
    void flush(CleanupFn cleanup = {}) {
        std::lock_guard<std::mutex> lock(mutex_);
        while (size_ > 0) {
            cleanup(queue_[rindex_]);
            rindex_ = wrap(rindex_ + 1);
            size_--;
        }
        windex_ = 0;
        rindex_ = 0;
        rindex_shown_ = 0;
        cond_.notify_one();
    }
};
