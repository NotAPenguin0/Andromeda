#pragma once

#include <andromeda/ecs/component_storage.hpp>
#include <andromeda/ecs/component_id.hpp>
#include <andromeda/ecs/component_view.hpp>
#include <andromeda/ecs/entity.hpp>

#include <cstdint>

#include <memory>
#include <vector>

namespace andromeda::ecs {

class registry {
public:
    registry();
    registry(registry const&) = delete;
    registry(registry&&) = default;
    
    registry& operator=(registry const&) = delete;
    registry& operator=(registry&&) = default;

    ~registry() = default;

    entity_t create_entity();

    template<typename T, typename... Args>
    T& add_component(entity_t entity, Args&&... args) {
        component_storage<T>& storage = get_or_emplace_storage<T>();
        auto it = storage.construct(entity, std::forward<Args>(args) ...);
        return *it;
    }

    template<typename T>
    bool has_component(entity_t entity) const {
        component_storage<T> const& storage = get_or_emplace_storage<T>();
        auto const it = storage.find(entity);
        return it != storage.end();
    }

    template<typename T>
    T& get_component(entity_t entity) {
        component_storage<T>& storage = get_or_emplace_storage<T>();
        return *storage.find(entity);
    }

    template<typename T>
    T const& get_component(entity_t entity) const {
        component_storage<T> const& storage = get_or_emplace_storage<T>();
        return *storage.find(entity);
    }

    // Counts all entities that have a specific set of components. Complexity is O(N) where N is size of the smallest container of all components
    // specified in the type list. Complexity is O(1) when sizeof...(Ts) == 1 or sizeof...(Ts) == 0
    template<typename... Ts>
    size_t count() const {
        if constexpr (sizeof...(Ts) == 0) { return 0; }
        if constexpr (sizeof...(Ts) == 1) { return get_or_emplace_storage<Ts...>().size(); } // sizeof...(Ts) == 1, so this instantiation is valid

        auto view = this->view<Ts...>();
        size_t n = 0;
        for (auto const& _ : view) {
            ++n;
        }
        return n;
    }
    

    template<typename... Ts>
    component_view<Ts...> view() {
        return { get_or_emplace_storage<Ts>() ... };
    }

    template<typename... Ts>
    const_component_view<Ts...> view() const {
        return { get_or_emplace_storage<Ts>() ... };
    }

    std::vector<entity_t> const& get_entities() const;

private:
    struct storage_data {
        uint64_t type_id = 0;
        std::unique_ptr<component_storage_base> storage;
    };

    struct entity_id_generator {
        entity_t cur = 0;

        entity_t next() {
            return cur++;
        }
    } id_generator;

    template<typename T>
    component_storage<T>& get_or_emplace_storage() {
        uint64_t const index = get_component_type_id<T>();

        // If the index is not found, we have to register the new component
        if (index >= storages.size()) {
            storages.resize(index + 1);
            storages[index].type_id = index;    
            storages[index].storage = std::make_unique<component_storage<T>>();
        }
        // Initialize storage if it wasn't created yet
        if (storages[index].storage == nullptr) {
            storages[index].type_id = index;
            storages[index].storage = std::make_unique<component_storage<T>>();
        }

        storage_data& storage = storages[index];
        return *static_cast<component_storage<T>*>(storage.storage.get());
    }

    template<typename T>
    component_storage<T> const& get_or_emplace_storage() const {
        uint64_t const index = get_component_type_id<T>();

        // If the index is not found, we have to register the new component
        if (index >= storages.size()) {
            storages.resize(index + 1);
            storages[index].type_id = index;    
            storages[index].storage = std::make_unique<component_storage<T>>();
        }
        // Initialize storage if it wasn't created yet
        if (storages[index].storage == nullptr) {
            storages[index].type_id = index;
            storages[index].storage = std::make_unique<component_storage<T>>();
        }

        storage_data const& storage = storages[index];
        return *static_cast<component_storage<T> const*>(storage.storage.get());
    }

    std::vector<entity_t> entities;
    mutable std::vector<storage_data> storages;
};

} // namespace andromeda::ecs
