module;

#include <cstdint>
#include <utility>

extern "C" {
#include <libavutil/dict.h>
}

export module AV.Dictionary;

export namespace AV
{
	class Dictionary
	{
		AVDictionary* m_dict{};
	public:
		Dictionary(Dictionary const&) = delete;
		constexpr Dictionary(Dictionary&& other) noexcept : m_dict{ std::exchange(other.m_dict, nullptr) }
		{
		}

		Dictionary& operator=(Dictionary const&) = delete;
		Dictionary& operator=(Dictionary&& other) noexcept;

		//Look up
		AVDictionaryEntry* get(const char* key, AVDictionaryEntry const* prev, int flags) const;
		AVDictionaryEntry const* iterate(AVDictionaryEntry const* prev) const;
		int count() const;

		//set
		int set(const char* key, const char* value, int flags);
		int set(const char* key, int64_t value, int flags);


		~Dictionary();
	};
}