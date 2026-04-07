module;
#include <array>

export module utils.RingBuffer;

export template<typename T, std::size_t Capacity>
class RingBuffer
{
protected:
    std::array<T, Capacity> queue_{};
    int rindex_ = 0;
    int windex_ = 0;
    int size_ = 0;

    static constexpr int wrap(int index) noexcept 
    {
        return index % static_cast<int>(Capacity);
    }

public:
    // types

    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;

    // construction

    constexpr explicit RingBuffer() noexcept = default;

    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    // capacity

    static constexpr size_type capacity() noexcept {
        return Capacity;
    }

    [[nodiscard]] int size() const noexcept {
        return size_;
    }

    [[nodiscard]] bool empty() const noexcept {
        return size_ == 0;
    }

    [[nodiscard]] bool full() const noexcept {
        return size_ >= static_cast<int>(Capacity);
    }
};
