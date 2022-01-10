#pragma once

#include <andromeda/util/handle.hpp>
#include <andromeda/thread/locked_value.hpp>
#include <andromeda/thread/scheduler.hpp>

#include <andromeda/app/log.hpp>
#include <andromeda/world.hpp>

#include <concepts>
#include <filesystem>
#include <mutex>
#include <string_view>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

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
    delay_storage_init(Status status, T data) : status{status}, data{std::move(data)} {}

    Status status;
    T data;
};

// Stores a single asset and its status.
template<typename T>
struct asset_storage_type {
    asset_storage_type(delay_storage_init<T>&& init) : status{init.status}, data{std::move(init.data)} {}

    std::atomic<Status> status;
    T data;

    // Additional information
    thread::task_id load_task_id = static_cast<thread::task_id>(-1);
    // Path to the asset file. This will be used to prevent loading the same asset multiple times.
    // This is stored as an absolute path
    fs::path path;
};

template<typename T>
using container = std::unordered_map<Handle<T>, asset_storage_type<T>>;

template<typename T>
container<T> data;

template<typename T>
std::mutex data_mutex;

// Global pointers to graphics context and world structure. This is to minimize the amount of parameters given to load() calls
extern gfx::Context* gfx_context;
extern World* world;

/**
 * @brief Get thread safe access to the asset storage for asset type T.
 * @tparam T The asset type to get access to.
 * @return A structure holding the container and a locked mutex that will be released on destruction.
*/
template<typename T>
thread::LockedValue<container<T>> acquire() {
    return thread::LockedValue<container<T>>{._lock = std::lock_guard{data_mutex<T>}, .value = data<T>};
}

/**
 * @brief Inserts a new empty asset in the pending state into the storage.
 * @tparam T Type of the asset to insert.
 * @param task Task ID of the load task. Defaults to -1 (no task).
 * @return Handle referring to the asset to delete.
*/
template<typename T>
Handle<T> insert_pending() requires std::default_initializable<T> && std::movable<T> {
    // Implementation similar to take()

    auto[_, storage] = acquire<T>();
    Handle<T> handle = Handle<T>::next();
    storage.emplace(handle, delay_storage_init<T>{Status::Pending, T{}});
    return handle;
}

/**
 * @brief Removes an asset from the system. Note that this does not free its resources, for this
 *		  you must call unload().
 * @tparam T The type of the asset to delete.
 * @param handle Handle referring to the asset to delete.
*/
template<typename T>
void delete_asset(Handle<T> handle) {
    auto[_, storage] = acquire<T>();
    storage.erase(handle);
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

    auto[_, storage] = acquire<T>();
    asset_storage_type<T>& element = storage.at(handle);
    element.status = Status::Ready;
    element.data = std::move(asset);
}

/**
 * @brief Sets the loading task associated with an asset
 * @tparam T Type of the asset.
 * @param handle Handle referring to the asset to set a load task of.
 * @param task Task ID of the load task.
*/
template<typename T>
void set_load_task(Handle<T> handle, thread::task_id task) {
    if (!handle) {
        LOG_WRITE(LogLevel::Error, "Tried to set load task of a null handle");
        return;
    }

    auto[_, storage] = acquire<T>();
    asset_storage_type<T>& element = storage.at(handle);
    element.load_task_id = task;
}

/**
 * @brief Query the loading task used to load this asset.
 * @tparam T Type of the asset.
 * @param handle Handle referring to the asset to query the load task of.
 * @return Task ID of the load task used to load this asset.
*/
template<typename T>
thread::task_id get_load_task(Handle<T> handle) {
    if (!handle) {
        LOG_WRITE(LogLevel::Error, "Tried to query load task of a null handle");
        return static_cast<thread::task_id>(-1);
    }

    auto[_, storage] = acquire<T>();
    asset_storage_type<T>& element = storage.at(handle);
    return element.load_task_id;
}

/**
 * @brief Sets the path of an asset.
 * @tparam T Type of the asset.
 * @param handle Handle referring to the asset to set the path of.
 * @param path Path of the asset on disk.
*/
template<typename T>
void set_path(Handle<T> handle, fs::path const& path) {
    if (!handle) {
        LOG_WRITE(LogLevel::Error, "Tried to set path of a null handle");
        return;
    }

    auto[_, storage] = acquire<T>();
    asset_storage_type<T>& element = storage.at(handle);
    element.path = fs::absolute(path);
}

/**
 * @brief Specialize this function for each type T assets need to be loaded.
 * @tparam T Type of the asset to load.
 * @param path Path to the asset file.
 * @return Handle referring to the loaded asset.
*/
template<typename T>
Handle<T> load_priv(std::string const& path);

/**
 * @brief The asset system uses a few global pointers to reduce common parameters to load functions
 * @param ctx Pointer to the graphics context.
 * @param wrld Pointer to the scene world.
 */
inline void set_global_pointers(gfx::Context* ctx, World* wrld) {
    gfx_context = ctx;
    world = wrld;
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
Handle<T> load(std::string const& path) {
    // First we look for the asset in the list of already loaded assets by comparing paths.
    {
        fs::path path_fs = fs::absolute(path);
        auto[_, storage] = impl::acquire<T>();
        for (auto const&[handle, element]: storage) {
            // Do not compare assets with empty paths (these can be created internally).
            if (element.path == "") { continue; }
            // Use equality compare because std::filesystem::equivalent is very expensive (and also errors on invalid paths, which we want to handle later in the pipeline).
            if (element.path == path_fs) { return handle; }
        }
    }

    // Asset wasn't found, we'll call the private load function to actually load it.
    Handle<T> handle = impl::load_priv<T>(path);
    return handle;
}

/**
 * @brief Unload an asset.
 * @tparam T The type of the asset to unload.
 * @param ctx Reference to the graphics context.
 * @param handle Handle referring to the asset to unload.
*/
template<typename T>
void unload(Handle<T> handle);

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
    auto[_, storage] = impl::acquire<T>();
    // Create a handle
    Handle<T> handle = Handle<T>::next();
    // Store the asset away
    storage.emplace(handle, impl::delay_storage_init<T>{Status::Ready, std::move(asset)});
    return handle;
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

    auto[_, storage] = impl::acquire<T>();
    impl::asset_storage_type<T>& element = storage.at(handle);
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

    auto[_, storage] = impl::acquire<T>();
    try {
        impl::asset_storage_type<T>& element = storage.at(handle);
        if (element.status != Status::Ready) {
            LOG_WRITE(LogLevel::Warning, "Tried to get asset while it is in the pending state.");
            return nullptr;
        }
        return &element.data;
    }
    catch (std::out_of_range const& not_found) {
        LOG_WRITE(LogLevel::Error, "Tried to get asset for invalid handle");
    }
    return nullptr;
}

/**
 * @brief Get the absolute path of an asset.
 * @tparam T Type of the asset.
 * @param handle Handle to the asset.
 * @return std::nullopt if no path was set or if the handle was null, the path otherwise.
 */
template<typename T>
std::optional<fs::path> get_path(Handle<T> handle) {
    if (!handle) { return std::nullopt; }

    auto[_, storage] = impl::acquire<T>();
    try {
        impl::asset_storage_type<T>& element = storage.at(handle);
        return element.path;
    }
    catch (std::out_of_range const& not_found) {
        return std::nullopt;
    }
}

/**
 * @brief Unloads all assets of a given type.
 * @tparam T Type of the assets to unload.
*/
template<typename T>
void unload_all() {
    // We need to collect all handles in a vector first so we can release the lock again, since
    // unload() indirectly tries to lock the asset system again.
    std::vector<Handle < T>>
    handles;
    {
        auto[_, storage] = impl::acquire<T>();
        handles.reserve(storage.size());
        for (auto&[handle, _]: storage) {
            handles.push_back(handle);
        }
    }
    for (auto handle: handles) {
        unload(handle);
    }
}

} // namespace assets
} // namespace andromeda