// This file is part of the andromeda reflection API for component types.
// Many files in this API are automatically generated from the component headers during build.
// Don't modify these manually, since the changes will be overwritten by the generation program.

// This header is not generated, but modifying it might result in unexpected behaviour.
// This header exposes the main reflection API, typically you only need to include this to use it.

// To obtain reflection information for a type that has it available, call meta::reflect<T>().
// This function returns a meta::reflection_info<T> struct with all the information. Example usage that prints all the members of a type:
// 
// This visitor only supports reflected structures that only have int and float fields.
// template<typename T>
// struct print_visitor {
//    T& instance;
//
//    void operator()(typed_field<T, int> const& value) {
//        std::cout << "(int) = " << value.get(instance);
//    }
//
//    void operator()(typed_field<T, float> const& value) {
//        std::cout << "(float) = " << value.get(instance);
//    }
// };
// 
// template<typename T>
// void print_members(T& instance) {
//    meta::reflection_info<T> refl = meta::reflect<T>();
// 
//    print_visitor<T> visitor{ instance };
//    for (auto const& field : refl.fields()) {
//        std::cout << field.name() << " ";
//        std::visit(visitor, field);
//        std::cout << std::endl;
//    }
// }


// To add reflection information for your own types, simply specialize the meta::reflect<T>() function for your type.
// 
// As an example, here is the meta::reflect() function specialized for an example struct A:
// struct A {
//    int x;
//    float y;
// };
// 
// namespace meta {
//    template<>
//    reflection_info<A> reflect<A>() {
//        return reflection_info("A", { typed_field<A>("x", &A::x), typed_field<A>("y", &A::y) });
//    }
// } 
//

#pragma once

#include <vector>
#include <variant>
#include <string>
#include <string_view>

// This included file is a generated file.
#include <reflect/type_lists.hpp>

namespace andromeda::meta {

/**
 * @struct typed_field 
 * @brief This structure stores a field of a reflected structure, with all meta information. It can also be used to get a reference to that field 
 *        for a specific instance.
 * @tparam T Type of the structure the field belongs to.
 * @tparam U Type of the field.
*/
template<typename T, typename U>
struct typed_field {
public:

    /**
     * @brief Construct a field with meta information.
     * @param ptr Pointer to member for the field to store. For example, in the following struct
     *            struct A { int x; float y; };
     *            this pointer is &A::x for x, and &A::y for y.
     * @param name String name of the field.
    */
    typed_field(U T::* ptr, std::string_view name) : ptr(ptr), field_name(std::string{ name }) {

    }

    /**
     * @brief Obtain a reference to the field in an instance.
     * @param instance Instance of the struct T to return the field for.
     * @return Reference to the stored field in the given instance.
    */
    U& get(T& instance) {
        return instance.*ptr;
    }

    /**
     * @brief Obtain a reference to the field in an instance.
     * @param instance Instance of the struct T to return the field for.
     * @return Reference to the stored field in the given instance.
    */
    U const& get(T const& instance) const {
        return instance.*ptr;
    }

    /**
     * @brief Get the string identifier of the field.
     * @return String with the name of the field. Lives as long as this typed_field does.
    */
    std::string const& name() const {
        return field_name;
    }

private:
    U T::* ptr = nullptr;
    std::string field_name;
};

namespace impl {

// Helper structure to define the type of field_variant from the type list.
template<typename T, typename... Us> 
struct construct_field_variant_type {
    using type = std::variant<typed_field<T, Us> ...>;
};

}

/**
 * @brief This type represents any field of a type T. It's a variant containing typed_fields for every possible field type.
 * @tparam T Structure type the fields are from.
 * @return 
*/
template<typename T>
using field_variant = typename impl::construct_field_variant_type<T, ANDROMEDA_META_FIELD_TYPES>::type;

/**
 * @brief This structure stores reflection info for a type T.
 * @tparam T The structure to store reflection info for.
*/
template<typename T>
struct reflection_info {
public:
    /**
     * @brief Create a reflection_info structure. It's recommended that these are stored somewhere instead of constructing them on the fly.
     * @param name Type name of the type T.
     * @param fields List of fields for type T.
    */
    reflection_info(std::string_view name, std::vector<field_variant<T>> fields) : type_name(std::string{ name }), type_fields(std::move(fields)) {

    }

    /**
     * @brief Obtain the type name.
     * @return A string with the type name. This string lives as long as the reflection_info structure does.
    */
    std::string const& name() const {
        return type_name;
    }

    /**
     * @brief Obtain the fields of the type.
     * @return A list of field_variant<T>, each with a single field. Lives as long as the reflection_info structure does.
    */
    std::vector<field_variant<T>> const& fields() const {
        return type_fields;
    }

private:
    std::string type_name;
    std::vector<field_variant<T>> type_fields;
};

/**
 * @brief Obtain reflection information for a type T.
 * @tparam T Type of the structure to obtain reflection info for.
*/
template<typename T>
reflection_info<T> reflect();

} // namespace andromeda::meta

// This is a generated header file containing declarations for specializations of reflect<T> for all components.
#include <reflect/component_reflect_decl.hpp>