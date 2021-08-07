#pragma once

#include <andromeda/util/idgen.hpp>

#include <cstdint>
#include <functional>

namespace andromeda {

/**
 * @struct Handle 
 * @brief Represents a unique handle to some type T
 * @tparam T The type this handle refers to.
*/
template<typename T>
struct Handle {

	/**
	 * @brief Constructs a handle as a null handle.
	*/
	Handle() = default;

	/**
	 * @brief Copies a handle.
	 * @param rhs The handle to copy.
	*/
	Handle(Handle const& rhs) = default;

	/**
	 * @brief Copies a handle.
	 * @param rhs The handle to copy.
	 * @return *this.
	*/
	Handle& operator=(Handle const& rhs) = default;

	/**
	 * @var static Handle none 
	 * @brief Represents a null handle.
	*/
	static Handle none;

	/**
	 * @brief Comparator for handles.
	 * @param rhs The handle to compare with.
	 * @return true if the handles refer to the same id.
	*/
	bool operator==(Handle const& rhs) const {
		return id == rhs.id;
	}

	/**
	 * @brief Converts a handle to a boolean
	 * @return true if the handle is not null, false otherwise.
	*/
	explicit operator bool() const {
		return id != none.id;
	}

	/**
	 * @brief Get the next availble handle of this type.
	 * @return A unique handle of the given type.
	*/
	static Handle next() {
		return Handle{ IDGen<T, uint64_t>::next() };
	}

private:
	friend class std::hash<Handle<T>>;

	uint64_t id = none.id;

	Handle(uint64_t id) : id(id) {}
};

template<typename T>
Handle<T> Handle<T>::none = Handle<T>{ static_cast<uint64_t>(-1) };

}

namespace std {

template<typename T>
struct hash<andromeda::Handle<T>> {
	size_t operator()(andromeda::Handle<T> const& x) const {
		hash<uint64_t> hash{};
		return hash(x.id);
	}
};

}