#ifndef ANDROMEDA_ASSETS_HPP_
#define ANDROMEDA_ASSETS_HPP_

#include <andromeda/core/context.hpp>
#include <andromeda/util/handle.hpp>

#include <stl/assert.hpp>

#include <atomic>
#include <mutex>
#include <unordered_map>

namespace andromeda {
namespace assets {

enum class Status {
	Pending,
	Ready
};

namespace storage {

// Thanks melak
template<typename T>
struct delay_storage_init {
	delay_storage_init(Status status, T data) : status{ status }, data{ std::move(data) } {}

	Status status;
	T data;
};

template<typename T>
struct storage_type {
	storage_type(delay_storage_init<T>&& init) : status{ init.status }, data{ std::move(init.data) } {}

	std::atomic<Status> status;
	T data;
};

template<typename T>
std::unordered_map<uint64_t, storage_type<T>> data;

template<typename T>
std::mutex data_mutex;

template<typename T>
struct lazy_atomic_init {
	T init;
	operator std::atomic<T>() const {
		return std::atomic<T>(init);
	}
};

}

template<typename T>
Handle<T> load(Context& ctx, std::string_view path);

template<typename T>
Handle<T> load(Context& ctx, std::string_view path, bool);

template<typename T>
Handle<T> take(T&& asset) {
	std::lock_guard lock(storage::data_mutex<T>);
	Handle<T> handle = Handle<T>::next();
	storage::data<T>.emplace(handle.id, storage::delay_storage_init<T>{ Status::Ready, std::move(asset) });
	return handle;
}

template<typename T>
Handle<T> insert_pending() {
	Handle<T> handle = take(T{});
	std::lock_guard lock(storage::data_mutex<T>);
	storage::data<T>.at(handle.id).status = Status::Pending;
	return handle;
}

template<typename T>
void finalize_load(Handle<T> handle, T&& asset) {
	std::lock_guard lock(storage::data_mutex<T>);
	storage::data<T>.at(handle.id).status = Status::Ready;
	storage::data<T>.at(handle.id).data = std::move(asset);
}

template<typename T>
bool is_ready(Handle<T> handle) {
	if (handle.id == Handle<T>::none) { return false; }
	return storage::data<T>.at(handle.id).status == Status::Ready;
}

template<typename T>
T* get(Handle<T> handle) {
	auto it = storage::data<T>.find(handle.id);
	if (it != storage::data<T>.end()) { 
		STL_ASSERT(it->second.status == Status::Ready, "Cannot get asset if it is not in the Ready state");
		return &it->second.data; 
	}
	return nullptr;
}

} // namespace assets
} // namespace andromeda

#endif