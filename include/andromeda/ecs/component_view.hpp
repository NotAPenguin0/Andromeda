#pragma once

#include <andromeda/ecs/component_storage.hpp>

#include <cassert>
#include <tuple>

namespace andromeda::ecs {

template<typename... Ts>
class component_view {
private:
    using view_type = std::tuple<component_storage < Ts>* ...>;
public:
    static_assert(sizeof...(Ts) > 0, "component_view must view at least one type.");

    class iterator {
    public:
        using value_type = std::tuple<Ts& ...>;

        iterator() = default;

        iterator(view_type& view, component_storage_base::iterator entity, component_storage_base::iterator end)
            : view(&view), entity(entity), end(end) {
            // Only do this if we're not at the end
            if (entity != end) {
                // If the current entity doesn't match
                if (!((std::get<component_storage<Ts>*>(view)->find(*entity) != std::get<component_storage<Ts>*>(view)->end()) && ...)) {
                    // Advance until we find a match, or end()
                    advance_to_next();
                }
            }
        }

        iterator(iterator const&) = default;

        iterator& operator=(iterator const&) = default;

        auto operator*() {
            assert(view && "Iterator pointing to invalid view");
            assert(entity != end && "Cannot dereference end iterator");

            return std::tie(std::get<component_storage < Ts> * > (*view)->get(*entity) ...);
        }

        iterator operator++() {
            advance_to_next();
            return *this;
        }

        iterator operator++(int) {
            iterator copy = *this;
            advance_to_next();
            return copy;
        }

        bool operator==(iterator other) const {
            return view == other.view && entity == other.entity;
        }

        bool operator!=(iterator other) const {
            return !(*this == other);
        }

    private:
        void advance_to_next() {
            ++entity;
            if (entity == end) { return; }
            while (
                !((std::get<component_storage<Ts>*>(*view)->find(*entity) != std::get<component_storage<Ts>*>(*view)->end()) && ...)
                ) {
                ++entity;
            }
        }

        view_type* view = nullptr;
        component_storage_base::iterator entity;
        component_storage_base::iterator end;
    };

    component_view(component_storage <Ts>& ... storages)
        : storages{&storages ...} {

        storage_to_check = find_smallest_storage();
    }

    iterator begin() {
        return iterator(storages, storage_to_check->begin(), storage_to_check->end());
    }

    iterator end() {
        return iterator(storages, storage_to_check->end(), storage_to_check->end());
    }

private:
    view_type storages;
    component_storage_base* storage_to_check;

    component_storage_base* find_smallest_storage() {
        return find_smallest_storage_impl<component_storage<Ts>* ...>();
    }

    template<typename Cur, typename Next, typename... Rest>
    component_storage_base* find_smallest_storage_impl(component_storage_base* previous = nullptr) {
        if (!previous) {
            // If we have no previous smallest storage, return the current one as the smallest.
            // This only happens on the first iteration
            return find_smallest_storage_impl<Next, Rest...>(std::get<Cur>(storages));
        }

        component_storage_base* cur_storage = std::get<Cur>(storages);
        return find_smallest_storage_impl<Next, Rest...>(
            cur_storage->size() < previous->size() ? cur_storage : previous
        );
    }

    template<typename Cur>
    component_storage_base* find_smallest_storage_impl(component_storage_base* previous = nullptr) {
        if (!previous) {
            return std::get<Cur>(storages);
        }

        component_storage_base* cur_storage = std::get<Cur>(storages);
        return cur_storage->size() < previous->size() ? cur_storage : previous;
    }
};


template<typename... Ts>
class const_component_view {
private:
    using view_type = std::tuple<component_storage < Ts> const* ...>;
public:
    static_assert(sizeof...(Ts) > 0, "component_view must view at least one type.");

    class iterator {
    public:
        using value_type = std::tuple<Ts const& ...>;

        iterator() = default;

        iterator(view_type& view, component_storage_base::iterator entity, component_storage_base::iterator end)
            : view(&view), entity(entity), end(end) {
            // Only do this if we're not at the end
            if (entity != end) {
                // If the current entity doesn't match
                if (!((std::get<component_storage<Ts> const*>(view)->find(*entity) != std::get<component_storage<Ts> const*>(view)->end()) && ...)) {
                    // Advance until we find a match, or end()
                    advance_to_next();
                }
            }
        }

        iterator(iterator const&) = default;

        iterator& operator=(iterator const&) = default;

        auto operator*() {
            assert(view && "Iterator pointing to invalid view");
            assert(entity != end && "Cannot dereference end iterator");

            return std::tie(std::get<component_storage < Ts> const* > (*view)->get(*entity) ...);
        }

        iterator operator++() {
            advance_to_next();
            return *this;
        }

        iterator operator++(int) {
            iterator copy = *this;
            advance_to_next();
            return copy;
        }

        bool operator==(iterator other) const {
            return view == other.view && entity == other.entity;
        }

        bool operator!=(iterator other) const {
            return !(*this == other);
        }

    private:
        void advance_to_next() {
            ++entity;
            if (entity == end) { return; }
            while (
                !((std::get<component_storage<Ts> const*>(*view)->find(*entity) != std::get<component_storage<Ts> const*>(*view)->end()) && ...)
                ) {
                ++entity;
            }
        }

        view_type* view = nullptr;
        component_storage_base::iterator entity;
        component_storage_base::iterator end;
    };

    const_component_view(component_storage <Ts> const& ... storages)
        : storages{&storages ...} {

        storage_to_check = find_smallest_storage();
    }

    iterator begin() {
        return iterator(storages, storage_to_check->begin(), storage_to_check->end());
    }

    iterator end() {
        return iterator(storages, storage_to_check->end(), storage_to_check->end());
    }

private:
    view_type storages;
    component_storage_base const* storage_to_check;

    component_storage_base const* find_smallest_storage() {
        return find_smallest_storage_impl<component_storage<Ts> const* ...>();
    }

    template<typename Cur, typename Next, typename... Rest>
    component_storage_base const* find_smallest_storage_impl(component_storage_base const* previous = nullptr) {
        if (!previous) {
            // If we have no previous smallest storage, return the current one as the smallest.
            // This only happens on the first iteration
            return find_smallest_storage_impl<Next, Rest...>(std::get<Cur>(storages));
        }

        component_storage_base const* cur_storage = std::get<Cur>(storages);
        return find_smallest_storage_impl<Next, Rest...>(
            cur_storage->size() < previous->size() ? cur_storage : previous
        );
    }

    template<typename Cur>
    component_storage_base const* find_smallest_storage_impl(component_storage_base const* previous = nullptr) {
        if (!previous) {
            return std::get<Cur>(storages);
        }

        component_storage_base const* cur_storage = std::get<Cur>(storages);
        return cur_storage->size() < previous->size() ? cur_storage : previous;
    }
};

}
