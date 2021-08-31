// THIS IS A GENERATED HEADER. DO NOT MODIFY.

// This header features type lists (simple macros defining lists of types) used in the reflection API.
// It also includes every component file.

#pragma once

#include <functional>

{{#includes}}
#include <andromeda/components/{{filename}}>
{{/includes}}

namespace andromeda::meta {

#define ANDROMEDA_META_FIELD_TYPES {{#field_types}}{{{type}}}{{comma}} {{/field_types}}
#define ANDROMEDA_META_COMPONENT_TYPES {{#component_types}}{{{type}}}{{comma}} {{/component_types}}


namespace detail {

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

} // namespace detail

template<template<typename> typename F, typename... Args>
void for_each_component(Args&&... args) {
    detail::for_each_component_impl<F, ANDROMEDA_META_COMPONENT_TYPES>{}(std::forward<Args>(args) ...);
}

} // namespace andromeda::meta