#ifndef ANDROMEDA_ASSETS_HPP_
#define ANDROMEDA_ASSETS_HPP_

#include <andromeda/core/context.hpp>
#include <andromeda/util/handle.hpp>

#include <unordered_map>

namespace andromeda {
namespace assets {

namespace storage {

template<typename T>
std::unordered_map<uint64_t, T> data;

}

template<typename T>
Handle<T> load(Context& ctx, std::string_view path);

template<typename T>
Handle<T> take(T&& asset) {
	Handle<T> handle = Handle<T>::next();
	storage::data<T>.emplace(handle.id, std::move(asset));
	return handle;
}

template<typename T>
T* get(Handle<T> handle) {
	auto it = storage::data<T>.find(handle.id);
	if (it != storage::data<T>.end()) { return &it->second; }
	return nullptr;
}

} // namespace assets
} // namespace andromeda

#endif