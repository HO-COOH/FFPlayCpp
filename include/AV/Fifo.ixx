module;

#include <cstddef>
#include <cassert>
#include <utility>
#include <type_traits>
extern "C" {
#include <libavutil/fifo.h>
}

export module AV.Fifo;

export namespace AV
{
	template<typename T>
		requires std::is_trivially_copyable_v<T>&& std::is_standard_layout_v<T>
	class Fifo
	{
		AVFifo* m_fifo{};
	public:
		using size_type = std::size_t;
		using reference = T&;
		using value_type = T;
		using const_reference = T const&;

		explicit Fifo(size_type initial_capacity = 1, bool auto_grow = true) : m_fifo{ av_fifo_alloc2(initial_capacity, sizeof(T), auto_grow ? AV_FIFO_FLAG_AUTO_GROW : 0) }
		{
		}

		Fifo(Fifo const&) = delete;
		Fifo& operator=(Fifo const&) = delete;

		Fifo(Fifo&& other) noexcept : m_fifo{ std::exchange(other.m_fifo, nullptr) }
		{
		}

		Fifo& operator=(Fifo&& other) noexcept
		{
			if (this != &other)
			{
				av_fifo_freep2(&m_fifo);
				m_fifo = std::exchange(other.m_fifo, nullptr);
			}
			return *this;
		}

		[[nodiscard]] size_type size() const noexcept
		{
			return av_fifo_can_read(m_fifo);
		}

		[[nodiscard]] size_type available() const noexcept
		{
			return av_fifo_can_write(m_fifo);
		}

		void reserve(size_type additional)
		{
			int ret = av_fifo_grow2(m_fifo, additional);
			assert(ret >= 0);
		}

		void set_auto_grow_limit(size_type max_elements) noexcept
		{
			av_fifo_auto_grow_limit(m_fifo, max_elements);
		}

		int push(T const& item)
		{
			int ret = av_fifo_write(m_fifo, &item, 1);
			assert(ret >= 0);
			return ret;
		}

		void push(T const* items, size_type count)
		{
			int ret = av_fifo_write(m_fifo, items, count);
			assert(ret >= 0);
		}

		T pop()
		{
			T item;
			int ret = av_fifo_read(m_fifo, &item, 1);
			assert(ret >= 0);
			return item;
		}

		bool try_pop(T& item)
		{
			return av_fifo_read(m_fifo, &item, 1) >= 0;
		}

		void pop(T* items, size_type count)
		{
			int ret = av_fifo_read(m_fifo, items, count);
			assert(ret >= 0);
		}

		T peek(size_t offset) const
		{
			T item;
			int ret = av_fifo_peek(m_fifo, &item, 1, offset);
			assert(ret >= 0);
			return item;
		}

		void peek(T* items, size_type count, size_type offset) const
		{
			int ret = av_fifo_peek(m_fifo, items, count, offset);
			assert(ret >= 0);
		}

		void drain(size_type count)
		{
			av_fifo_drain2(m_fifo, count);
		}

		void reset()
		{
			av_fifo_reset2(m_fifo);
		}

		~Fifo()
		{
			av_fifo_freep2(&m_fifo);
		}
	};
}
