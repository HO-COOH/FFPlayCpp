module;
extern "C" {
#include <libavutil/rational.h>
}
#include <tuple>
#include <compare>
export module AV.Rational;

export namespace AV
{
	struct Rational : AVRational
	{
		constexpr Rational() = default;
		constexpr Rational(int num, int den) : AVRational{ num, den } {}
		constexpr Rational(AVRational value) : AVRational{ value } {}
		constexpr operator AVRational() const { return *this; }

		constexpr Rational operator+(const Rational& rhs) const { return av_add_q(*this, rhs); }
		constexpr Rational operator-(const Rational& rhs) const { return av_sub_q(*this, rhs); }
		constexpr Rational operator*(const Rational& rhs) const { return av_mul_q(*this, rhs); }
		constexpr Rational operator/(const Rational& rhs) const { return av_div_q(*this, rhs); }

		constexpr Rational inv() const
		{
			return Rational{ den, num };
		}

		operator double() const
		{
			return av_q2d(*this);
		}

		std::partial_ordering operator<=>(Rational const& rhs) const
		{
			int const result = av_cmp_q(*this, rhs);
			if (result == 0) return std::partial_ordering::equivalent;
			if (result == INT_MIN) return std::partial_ordering::unordered;
			if (result < 0) return std::partial_ordering::less;
			return std::partial_ordering::greater;
		}
	};
}
