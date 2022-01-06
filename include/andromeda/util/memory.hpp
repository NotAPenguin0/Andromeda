#pragma once

#include <span>
#include <vector>

namespace andromeda::util {

/**
 * @brief Get the size of a vector's stored data in bytes.
 * @tparam T Element type of the vector.
 * @param v Vector to get the size of. May be empty.
 * @return Size in bytes of a vector
 */
template<typename T>
size_t memsize(std::vector<T> const& v) {
    return v.size() * sizeof(T);
}

/**
 * @brief Get the size of a span's stored data in bytes.
 * @tparam T Element type of the span.
 * @param v Vector to get the size of. May be empty.
 * @return Size in bytes of a span
 */
template<typename T>
size_t memsize(std::span<T> const& v) {
    return v.size() * sizeof(T);
}

} // namespace andromeda::util