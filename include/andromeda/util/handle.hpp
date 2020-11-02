#ifndef ANDROMEDA_HANDLE_HPP_
#define ANDROMEDA_HANDLE_HPP_

#include <andromeda/util/types.hpp>
#include <atomic>

namespace andromeda {

template<typename T>
struct Handle {
	static constexpr uint64_t none = static_cast<uint64_t>(-1);
	uint64_t id = none;

	static Handle next() {
		return Handle{ .id = cur++ };
	}

	inline explicit operator bool() const {
		return id != none;
	}

private:
	static inline std::atomic<uint64_t> cur = 0;
};

}

#endif