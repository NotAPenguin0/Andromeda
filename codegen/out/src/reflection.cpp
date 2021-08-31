// THIS IS A GENERATED SOURCE FILE. DO NOT MODIFY.

#include <reflect/reflection.hpp>

// Provide implementations for meta::reflect<T>() for component types.

namespace andromeda::meta {

template<>
reflection_info<Camera> reflect<Camera>() {
	return reflection_info<Camera>(
		"Camera",
		{
			typed_field<Camera, float>(&Camera::fov, "fov"),
			typed_field<Camera, float>(&Camera::near, "near"),
			typed_field<Camera, float>(&Camera::far, "far")
		}
	);
}

template<>
reflection_info<Hierarchy> reflect<Hierarchy>() {
	return reflection_info<Hierarchy>(
		"Hierarchy",
		{
			typed_field<Hierarchy, ecs::entity_t>(&Hierarchy::this_entity, "this_entity"),
			typed_field<Hierarchy, ecs::entity_t>(&Hierarchy::parent, "parent"),
			typed_field<Hierarchy, std::vector<ecs::entity_t>>(&Hierarchy::children, "children")
		}
	);
}

template<>
reflection_info<MeshRenderer> reflect<MeshRenderer>() {
	return reflection_info<MeshRenderer>(
		"MeshRenderer",
		{
			typed_field<MeshRenderer, Handle<gfx::Mesh>>(&MeshRenderer::mesh, "mesh")
		}
	);
}

template<>
reflection_info<Name> reflect<Name>() {
	return reflection_info<Name>(
		"Name",
		{
			typed_field<Name, std::string>(&Name::name, "name")
		}
	);
}

template<>
reflection_info<Transform> reflect<Transform>() {
	return reflection_info<Transform>(
		"Transform",
		{
			typed_field<Transform, glm::vec3>(&Transform::position, "position"),
			typed_field<Transform, glm::vec3>(&Transform::rotation, "rotation"),
			typed_field<Transform, glm::vec3>(&Transform::scale, "scale")
		}
	);
}


}