// THIS IS A GENERATED SOURCE FILE. DO NOT MODIFY.

#include <reflect/reflection.hpp>

// Provide implementations for meta::reflect<T>() for component types.

namespace andromeda::meta {

template<>
reflection_info<Camera> const& reflect<Camera>() {
	static reflection_info<Camera> refl_info = reflection_info<Camera>(
		"Camera",
		{
			field<Camera>(&Camera::fov, "fov", "Camera field of view in degrees."),
			field<Camera>(&Camera::near, "near", "Distance to the near clipping plane."),
			field<Camera>(&Camera::far, "far", "Distance to the far clipping plane.")
		},
		plib::bit_flag<type_flags>(type_flags::none) 
	);
	return refl_info;
}

template<>
reflection_info<Hierarchy> const& reflect<Hierarchy>() {
	static reflection_info<Hierarchy> refl_info = reflection_info<Hierarchy>(
		"Hierarchy",
		{
			field<Hierarchy>(&Hierarchy::this_entity, "this_entity", ""),
			field<Hierarchy>(&Hierarchy::parent, "parent", ""),
			field<Hierarchy>(&Hierarchy::children, "children", "")
		},
		plib::bit_flag<type_flags>(type_flags::none) |
		plib::bit_flag<type_flags>(type_flags::editor_hide) 
	);
	return refl_info;
}

template<>
reflection_info<MeshRenderer> const& reflect<MeshRenderer>() {
	static reflection_info<MeshRenderer> refl_info = reflection_info<MeshRenderer>(
		"MeshRenderer",
		{
			field<MeshRenderer>(&MeshRenderer::mesh, "mesh", "Mesh asset to render.")
		},
		plib::bit_flag<type_flags>(type_flags::none) 
	);
	return refl_info;
}

template<>
reflection_info<Name> const& reflect<Name>() {
	static reflection_info<Name> refl_info = reflection_info<Name>(
		"Name",
		{
			field<Name>(&Name::name, "name", "")
		},
		plib::bit_flag<type_flags>(type_flags::none) |
		plib::bit_flag<type_flags>(type_flags::editor_hide) 
	);
	return refl_info;
}

template<>
reflection_info<Transform> const& reflect<Transform>() {
	static reflection_info<Transform> refl_info = reflection_info<Transform>(
		"Transform",
		{
			field<Transform>(&Transform::position, "position", "Position relative to parent."),
			field<Transform>(&Transform::rotation, "rotation", "Rotation relative to parent, in degrees"),
			field<Transform>(&Transform::scale, "scale", "Scale relative to parent.")
		},
		plib::bit_flag<type_flags>(type_flags::none) 
	);
	return refl_info;
}


}