// GENERATED HEADER FILE. DO NOT MODIFY.
// This file is part of the andromeda reflection API and should only be included through <reflect/reflection.hpp>

#pragma once

#include <stdexcept>

namespace andromeda::meta {

/**
 * @brief Dispatches a function to a field pointer.
 *        This function will:
 *		     1) Look up the type of the field using field<T>::type()
 *           2) Based on this type, call field<T>::as<U>(instance) to get a reference x to the given field
 *              in the instance.
 *           3) Call func(x, args...)
*/
template<typename F, typename T, typename... Args>
void dispatch(field<T>& field, T& instance, F&& func, Args&&... args) {
	uint32_t const type_id = field.type();
	switch(type_id) {
		case 0:
			func(field.as<Handle<gfx::Mesh>>(instance), std::forward<Args>(args)...);
			break;
		case 1:
			func(field.as<ecs::entity_t>(instance), std::forward<Args>(args)...);
			break;
		case 2:
			func(field.as<float>(instance), std::forward<Args>(args)...);
			break;
		case 3:
			func(field.as<glm::vec3>(instance), std::forward<Args>(args)...);
			break;
		case 4:
			func(field.as<std::string>(instance), std::forward<Args>(args)...);
			break;
		case 5:
			func(field.as<std::vector<ecs::entity_t>>(instance), std::forward<Args>(args)...);
			break;
		default:
			throw std::runtime_error("Reflection missing type information for field type");
	}
}

/**
 * @brief Dispatches a function to a field pointer.
 *        This function will:
 *		     1) Look up the type of the field using field<T>::type()
 *           2) Based on this type, call field<T>::as<U>(instance) to get a const reference x to the given field
 *              in the instance.
 *           3) Call func(x, args...)
*/
template<typename F, typename T, typename... Args>
void dispatch(field<T> const& field, T const& instance, F&& func, Args&&... args) {
	uint32_t const type_id = field.type();
	switch(type_id) {
		case 0:
			func(field.as<Handle<gfx::Mesh>>(instance), std::forward<Args>(args)...);
			break;
		case 1:
			func(field.as<ecs::entity_t>(instance), std::forward<Args>(args)...);
			break;
		case 2:
			func(field.as<float>(instance), std::forward<Args>(args)...);
			break;
		case 3:
			func(field.as<glm::vec3>(instance), std::forward<Args>(args)...);
			break;
		case 4:
			func(field.as<std::string>(instance), std::forward<Args>(args)...);
			break;
		case 5:
			func(field.as<std::vector<ecs::entity_t>>(instance), std::forward<Args>(args)...);
			break;
		default:
			throw std::runtime_error("Reflection missing type information for field type");
	}
}

} // namespace andromeda::meta