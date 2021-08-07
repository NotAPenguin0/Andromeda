#pragma once

#include <andromeda/util/handle.hpp>
#include <andromeda/thread/locked_value.hpp>

#include <andromeda/app/log.hpp>

#include <concepts>
#include <mutex>
#include <string_view>
#include <unordered_map>


namespace andromeda {

namespace gfx {
class Context;
} // namespace gfx

namespace assets {

/**
 * @enum Status 
 * @brief Status of an asset. As long as an asset is in the Pending status it cannot be used.
*/
enum class Status {
	Ready, 
	Pending
};

// Private storage of the asset system. Do not touch.
namespace impl {

// Initializer for storage_type
template<typename T>
struct delay_storage_init {
	delay_storage_init(Status status, T data) : status{ status }, data{ std::move(data) } {}

	Status status;
	T data;
};

// Stores a single asset and its status.
template<typename T>
struct asset_storage_type {
	asset_storage_type(delay_storage_init<T>&& init) : status{ init.status }, data{ std::move(init.data) } {}

	std::atomic<Status> status;
	T data;
};

template<typename T>
using container = std::unordered_map<Handle<T>, asset_storage_type<T>>;

template<typename T>
container<T> data;

template<typename T>
std::mutex data_mutex;

/**
 * @brief Get thread safe access to the asset storage for asset type T.
 * @tparam T The asset type to get access to.
 * @return A structure holding the container and a locked mutex that will be released on destruction.
*/
template<typename T>
thread::LockedValue<container<T>> acquire() {
	return thread::LockedValue<container<T>>{ ._lock = std::lock_guard{ data_mutex<T> }, .value = data<T> };
}

} // namespace impl

/**
 * @brief Load an asset. This may happen asynchronously, so always check for availability
 *		  before using the asset.
 * @tparam T Type of the asset to load
 * @param ctx Reference to the graphics context.
 * @param path Path to the asset file.
 * @return Handle referring to the loaded asset.
*/
template<typename T>
Handle<T> load(gfx::Context& ctx, std::string_view path);

/**
 * @brief Consumes an asset object. This will store it inside the asset system and return a handle
 *		  for lookup.
 * @tparam T Type of the asset to consume.
 * @param asset Asset to consume.
 * @return Handle referring to the consumed asset.
*/
template<typename T>
Handle<T> take(T asset) requires std::movable<T> {
	// Get thread-safe access
	auto [_, storage] = impl::acquire<T>();
	// Create a handle
	Handle<T> handle = Handle<T>::next();
	// Store the asset away
	storage.emplace(handle, impl::delay_storage_init<T>{ Status::Ready, std::move(asset) });
	return handle;
}

/**
 * @brief Inserts a new empty asset in the pending state into the storage.
 * @tparam T Type of the asset to insert.
 * @return Handle referring to the inserted asset
*/
template<typename T>
Handle<T> insert_pending() requires std::default_initializable<T> && std::movable<T> {
	// Implementation similar to take()

	auto [_, storage] = impl::acquire<T>();
	Handle<T> handle = Handle<T>::next();
	storage.emplace(handle, impl::delay_storage_init<T>{Status::Pending, T{}});
	return handle;
}

/**
 * @brief Marks a previously pending asset as ready and stores it away.
 * @tparam T Type of the asset to insert.
 * @param handle Handle of the pending asset.
 * @param asset Asset that will be stored at the handle's location.
*/
template<typename T>
void make_ready(Handle<T> handle, T asset) requires std::movable<T> {
	if (!handle) {
		LOG_WRITE(LogLevel::Error, "Tried to mark null handle as ready");
		return;
	}

	auto [_, storage] = impl::acquire<T>();
	impl::asset_storage_type& element = storage.at(handle);
	element.status = Status::Ready;
	element.data = std::move(asset);
}

/**
 * @brief Query if an asset is in the Ready state.
 * @tparam T Type of the asset.
 * @param handle Handle of the asset to query.
 * @return true if the asset is in the Ready state, false if it is Pending.
*/
template<typename T>
bool is_ready(Handle<T> handle) {
	if (!handle) {
		LOG_WRITE(LogLevel::Error, "Tried to query if null handle is ready.");
		return false;
	}

	auto [_, storage] = impl::acquire<T>();
	impl::asset_storage_type& element = storage.at(handle);
	return element.status == Status::Ready;
}

/**
 * @brief Get the asset a handle refers to.
 * @tparam T Type of the asset.
 * @param handle Handle of the asset to get.
 * @return Pointer to the asset, or nullptr on failure.
*/
template<typename T>
T* get(Handle<T> handle) {
	if (!handle) {
		LOG_WRITE(LogLevel::Error, "Tried to get asset for null handle");
		return nullptr;
	}

	auto [_, storage] = impl::acquire<T>();
	try {
		impl::asset_storage_type& element = storage.at(handle);
		if (element.status != Status::Ready) {
			LOG_WRITE(LogLevel::Warning, "Tried to get asset while it is in the pending state.");
			return nullptr;
		}
		return &element.data;
	}
	catch (std::out_of_range not_found) {
		LOG_WRITE(LogLevel::Error, "Tried to get asset for invalid handle");
	}
	return nullptr;
}

} // namespace assets
} // namespace andromeda