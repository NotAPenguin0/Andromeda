#ifndef ANDROMEDA_HANDLE_HPP_
#define ANDROMEDA_HANDLE_HPP_

#include <andromeda/util/types.hpp>

namespace andromeda {

template<typename T>
struct Handle {
	static constexpr uint64_t none = static_cast<uint64_t>(-1);
	uint64_t id = none;
};

}

#endif