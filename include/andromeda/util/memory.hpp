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

/**
 * @brief Align a size to a certain alignment.
 * @tparam T Integral type representing a size.
 * @param size Possibly unaligned size.
 * @param alignment Request alignment.
 * @return A new size, aligned to the requested alignment. The result will never be smaller than the given size.
 */
template<typename T>
T align_size(T size, T alignment) {
    T const unaligned_size = size % alignment;
    T const padding = alignment - unaligned_size;
    return size + padding;
}


/**
 * @brief Converts a size in bytes to a size in KiB.
 * @tparam T Integral type representing a byte size.
 * @param size Size in bytes of some buffer.
 * @return Floating point value with the given byte size in KiB.
 */
template<typename T>
float bytes_to_KiB(T size) {
    return static_cast<float>(size) / 1024.0f;
}

/**
 * @brief Converts a size in bytes to a size in MiB.
 * @tparam T Integral type representing a byte size.
 * @param size Size in bytes of some buffer.
 * @return Floating point value with the given byte size in MiB.
 */
template<typename T>
float bytes_to_MiB(T size) {
    return static_cast<float>(size) / (1024.0f * 1024.0f);
}


} // namespace andromeda::util