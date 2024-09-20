#include <iostream>
#include "fixedpoint.hpp"
#include <cstdint>

int
main(void)
{
	double value1 = 3.1416;
	double value2 = 1.0;

	FixedPoint<int32_t, 12> fvalue1(value1);
	FixedPoint<int32_t, 12> fvalue2(value2);

	std::cout << "Value 1: " << value1 << " -> " << fvalue1.getRaw() << std::endl
		<< "Value 2: " << value2 << " -> " << fvalue2.getRaw() << std::endl;
	return 0;
}
