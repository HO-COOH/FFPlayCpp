module;
#include <variant>
#include <cstdint>
export module ffplay:SeekRequest;

export struct SeekByBytes
{
	int64_t pos;
	int64_t relative;
};

export struct SeekByTime
{
	int64_t pos;
	int64_t relative;
};

export using SeekRequest = std::variant<std::monostate, SeekByBytes, SeekByTime>;