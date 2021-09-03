// THIS IS A GENERATED HEADER. DO NOT MODIFY.

// This header features type lists (simple macros defining lists of types) used in the reflection API.
// It also includes every component file.

#pragma once

#include <functional>
#include <cstdint>

#include <andromeda/components/camera.hpp>
#include <andromeda/components/hierarchy.hpp>
#include <andromeda/components/mesh_renderer.hpp>
#include <andromeda/components/name.hpp>
#include <andromeda/components/transform.hpp>

namespace andromeda::meta {

#define ANDROMEDA_META_COMPONENT_TYPES Camera, Hierarchy, MeshRenderer, Name, Transform 

namespace impl {

template<template<typename> typename, typename...>
struct for_each_component_impl;

template<template<typename> typename F, typename CFirst>
struct for_each_component_impl<F, CFirst> {
    template<typename... Args>
    void operator()(Args&&... args) {
        F<CFirst>{}(std::forward<Args>(args) ...);
    }
};

template<template<typename> typename F, typename CFirst, typename CNext, typename... CRest>
struct for_each_component_impl<F, CFirst, CNext, CRest ...> {
    template<typename... Args>
    void operator()(Args&&... args) {
        F<CFirst>{}(std::forward<Args>(args) ...);
        for_each_component_impl<F, CNext, CRest...>{}(std::forward<Args>(args) ...);
    }
};

} // namespace impl

template<template<typename> typename F, typename... Args>
void for_each_component(Args&&... args) {
    impl::for_each_component_impl<F, ANDROMEDA_META_COMPONENT_TYPES>{}(std::forward<Args>(args) ...);
}


namespace impl {

template<typename T>
uint32_t type_id();

template<>
inline uint32_t type_id<Handle<gfx::Mesh>>() {
    return 0;
}

template<>
inline uint32_t type_id<ecs::entity_t>() {
    return 1;
}

template<>
inline uint32_t type_id<float>() {
    return 2;
}

template<>
inline uint32_t type_id<glm::vec3>() {
    return 3;
}

template<>
inline uint32_t type_id<std::string>() {
    return 4;
}

template<>
inline uint32_t type_id<std::vector<ecs::entity_t>>() {
    return 5;
}


} // namespace impl

} // namespace andromeda::meta