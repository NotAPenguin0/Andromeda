// THIS IS A GENERATED SOURCE FILE. DO NOT MODIFY.

#include <reflect/reflection.hpp>

// Provide implementations for meta::reflect<T>() for component types.

namespace andromeda::meta {

{{#impl}}
template<>
reflection_info<{{component}}> const& reflect<{{component}}>() {
	static reflection_info<{{component}}> refl_info = reflection_info<{{component}}>(
		"{{component}}",
		{
			{{#fields}}
			field<{{component}}>(
				&{{component}}::{{name}}, 
				"{{name}}", 
				"{{{tooltip}}}",
				{{#flags}}
				plib::bit_flag<field_flags>(field_flags::{{flag}}) {{or}}
				{{/flags}},
				field<{{component}}>::editor_values {
					.min = {{{min}}},
					.max = {{{max}}},
					.drag_speed = {{{drag_speed}}}
				}
			){{comma}}
			{{/fields}}
		},
		{{#flags}}
		plib::bit_flag<type_flags>(type_flags::{{flag}}) {{or}}
		{{/flags}}
	);
	return refl_info;
}

{{/impl}}

}