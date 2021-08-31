// THIS IS A GENERATED SOURCE FILE. DO NOT MODIFY.

#include <reflect/reflection.hpp>

// Provide implementations for meta::reflect<T>() for component types.

namespace andromeda::meta {

{{#impl}}
template<>
reflection_info<{{component}}> reflect<{{component}}>() {
	return reflection_info<{{component}}>(
		"{{component}}",
		{
			{{#fields}}
			typed_field<{{component}}, {{{type}}}>(&{{component}}::{{name}}, "{{name}}"){{comma}}
			{{/fields}}
		}
	);
}

{{/impl}}

}