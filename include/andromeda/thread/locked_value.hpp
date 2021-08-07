#pragma once

#include <mutex>

namespace andromeda {
namespace thread {

/**
	* @struct LockedValue
	* @brief Structure to represent a locked mutex + reference. Can be used to return internal objects
	*		  in a thread-safe way.
	* @tparam T Type of the stored value.
*/
template<typename T>
struct LockedValue {
	std::lock_guard<std::mutex> _lock;
	T& value;
};

} // namespace thread
} // namespace andromeda