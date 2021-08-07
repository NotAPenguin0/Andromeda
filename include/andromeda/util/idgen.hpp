#pragma once

#include <atomic>
#include <cstdint>

namespace andromeda {

template<typename T, typename IdType = uint32_t>
struct IDGen {
private:
	static inline std::atomic<IdType> cur = 0;
public:
	static IdType next() {
		return cur++;
	}
};

}