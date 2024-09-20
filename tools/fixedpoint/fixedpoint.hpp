#ifndef FIXEDPOINT_HPP
#define FIXEDPOINT_HPP

#include <cstdlib>
#include <cstdint>

template<class Type, size_t Scale>
class FixedPoint
{
private:
	const static Type factor = 1 << Scale;
	Type data;
public:
	FixedPoint(double value) {
		*this = value;
	}

	FixedPoint&
	operator=(double value) {
		this->data = static_cast<Type>(value * factor);
		return *this;
	}

	Type
	getRaw() const {
		return this->data;
	}
};

typedef FixedPoint<int32_t, 12> psxfixed32;
typedef FixedPoint<int16_t, 12> psxfixed16;

#endif

