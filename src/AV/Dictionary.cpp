module;

#include <utility>
#include <cstdint>
extern "C" {
#include <libavutil/dict.h>
}

module AV.Dictionary;

namespace AV
{
    Dictionary& Dictionary::operator=(Dictionary&& other) noexcept
    {
        if (this != &other)
        {
            av_dict_free(&m_dict);
            m_dict = std::exchange(other.m_dict, nullptr);
        }
        return *this;
    }

    AVDictionaryEntry* Dictionary::get(const char* key, AVDictionaryEntry const* prev, int flags) const
    {
        return av_dict_get(m_dict, key, prev, flags);
    }

    AVDictionaryEntry const* Dictionary::iterate(AVDictionaryEntry const* prev) const
    {
        return av_dict_iterate(m_dict, prev);
    }

    int Dictionary::count() const
    {
        return av_dict_count(m_dict);
    }

    int Dictionary::set(const char* key, const char* value, int flags)
    {
        return av_dict_set(&m_dict, key, value, flags);
    }

    int Dictionary::set(const char* key, int64_t value, int flags)
    {
        return av_dict_set_int(&m_dict, key, value, flags);
    }

    Dictionary::~Dictionary()
    {
        if (m_dict)
        {
            av_dict_free(&m_dict);
        }
    }
}
