#ifndef BIT_H
#define BIT_H

#include <cstdint>

namespace bit {

template<typename T>
inline uint8_t popcnt(T val) {
	uint8_t ans = 0;
	while(val) {
		val &= val - 1;
		++ans;
	}
	return ans;
}

template<typename T, typename T2>
inline bool has_invalid_bits(T available, T2 selected) {
	return (selected & (~available)) != 0;
}

}

#endif //BIT_H
