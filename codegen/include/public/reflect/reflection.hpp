// This file is part of the andromeda reflection API for component types.
// Many files in this API are automatically generated from the component headers during build.
// Don't modify these manually, since the changes will be overwritten by the generation program.
// This header is not generated, but modifying it might result in unexpected behaviour.
// 
// This header exposes the main reflection API, typically you only need to include this to use it.
// Note that the reflection API is designed to work with component types within the editor, 
// it's not meant to be a generic solution for reflection.
// 
// To obtain reflection information for a type that has it available, call meta::reflect<T>().
// This function returns a meta::reflection_info<T> struct with all the information.


#pragma once

#include <vector>
#include <variant>
#include <string>
#include <string_view>

#include <plib/bit_flag.hpp>

// This included file is a generated file.
#include <reflect/type_lists.hpp>

namespace andromeda::meta {

enum class field_flags : uint32_t {
    none = 0x00,
    editor_hide = 0x01,
    no_limits = 0x02
};

/**
 * @struct field 
 * @brief This structure stores a field of a reflected structure, with all meta information. It can also be used to get a reference to that field 
 *        for a specific instance.
 * @tparam T Type of the structure the field belongs to.
*/
template<typename T>
struct field {
public:
    struct editor_values {
        float min{};
        float max{};

        float drag_speed{};
    };

    field() = default;

    /**
     * @brief Construct a field with meta information.
     * @tparam U type of the field.
     * @param ptr Pointer to member for the field to store. For example, in the following struct
     *            struct A { int x; float y; };
     *            this pointer is &A::x for x, and &A::y for y.
     * @param name String name of the field.
     * @param tooltip String to display when hovering over the field in the editor.
    */
    template<typename U>
    field(U T::* ptr, std::string_view name, std::string_view tooltip, plib::bit_flag<field_flags> flags,
          editor_values values = {})
        : field_name(std::string{name}), field_tooltip(std::string{tooltip}), flags_(flags), values(values) {
        this->ptr = reinterpret_cast<void_t T::*>(ptr);
        this->field_type = impl::type_id<U>();
    }

    /**
     * @brief Test whether the field contains a valid pointer.
     * @return True if the field is valid and may be dereferenced using as<U>(), false otherwise.
     */
    [[nodiscard]] bool valid() const {
        return ptr != nullptr;
    }

    /**
     * @brief Test whether the field contains a valid pointer.
     * @return True if the field is valid and may be dereferenced using as<U>(), false otherwise.
     */
    [[nodiscard]] explicit operator bool() const {
        return valid();
    }

    /**
     * @brief Obtain a reference to the field in an instance.
     * @tparam U the type of the field.
     * @param instance Instance of the struct T to return the field for.
     * @return Reference to the stored field in the given instance.
    */
    template<typename U>
    U& as(T& instance) {
        auto typed = as_type<U>();
        return instance.*typed;
    }

    /**
     * @brief Obtain a reference to the field in an instance.
     * @tparam U the type of the field.
     * @param instance Instance of the struct T to return the field for.
     * @return Reference to the stored field in the given instance.
    */
    template<typename U>
    U const& as(T const& instance) const {
        auto const typed = as_type<U>();
        return instance.*typed;
    }

    /**
     * @brief Get the string identifier of the field.
     * @return String with the name of the field. Lives as long as this field does.
    */
    std::string const& name() const {
        return field_name;
    }

    /**
     * @brief Get a tooltip string for the field.
     * @return String with the tooltip for the field. Lives as long as this field does.
    */
    std::string const& tooltip() const {
        return field_tooltip;
    }

    /**
     * @brief Get the type id of the field.
     * @return Unique identifier for the type of this field.
    */
    uint32_t type() const {
        return field_type;
    }

    plib::bit_flag<field_flags> flags() const {
        return flags_;
    }

    float const& min() const {
        return values.min;
    }

    float const& max() const {
        return values.max;
    }

    float const& drag_speed() const {
        return values.drag_speed;
    }

private:
    // We define this struct so we can have a generic pointer type without having to cast to types like unsigned char*.
    // We can't use void because a pointer to a void member is not allowed.
    struct void_t {
    };

    void_t T::* ptr = nullptr;
    std::string field_name{};
    std::string field_tooltip{};
    plib::bit_flag<field_flags> flags_;
    editor_values values{};

    // For each unique type in the components we will assign an ID. 
    // This ID is then used in the dispatch function.
    uint32_t field_type = 0;

    template<typename U>
    U T::* as_type() {
        return reinterpret_cast<U T::*>(ptr);
    }

    template<typename U>
    U const T::* as_type() const {
        return reinterpret_cast<U const T::*>(ptr);
    }
};

enum class type_flags : uint32_t {
    none = 0x00,
    editor_hide = 0x01
};

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
    reflection_info(std::string_view name, std::vector<field<T>> fields, plib::bit_flag<type_flags> flags = {})
        : type_name(std::string{name}), type_fields(std::move(fields)), flags_(flags), invalid_field() {

    }

    /**
     * @brief Obtain the type name.
     * @return A string with the type name. This string lives as long as the reflection_info structure does.
    */
    [[nodiscard]] std::string const& name() const {
        return type_name;
    }

    /**
     * @brief Obtain the fields of the type.
     * @return A list of field<T>, each with a single field. Lives as long as the reflection_info structure does.
    */
    std::vector<field<T>> const& fields() const {
        return type_fields;
    }

    /**
     * @brief Obtain a field of the type with a given name. Returns an invalid field if none was found.
     * @param name
     * @return
     */
    field<T> const& field_with_name(std::string_view name) const {
        for (auto const& field: type_fields) {
            if (field.name() == name) { return field; }
        }

        return invalid_field;
    }

    /**
     * @brief Obtain the flags of this type.
     * @return A bit flag type representing this type's flags.
     */
    plib::bit_flag<type_flags> flags() const {
        return flags_;
    }

private:
    std::string type_name;
    std::vector<field<T>> type_fields;
    field<T> invalid_field;
    plib::bit_flag<type_flags> flags_;
};

/**
 * @brief Obtain reflection information for a type T.
 * @tparam T Type of the structure to obtain reflection info for.
*/
template<typename T>
reflection_info<T> const& reflect();

} // namespace andromeda::meta

// This is a generated header file containing declarations for specializations of reflect<T> for all components.
#include <reflect/component_reflect_decl.hpp>

// This is a generated file containing the implementation for dispatch() a function that will extract the type information from a meta::field<T>
// and use it to correctly cast to the proper type using the .as<U>() function, and then call a visitor on it.
// Do not include this header manually, as it depends on this header being included.
#include <reflect/dispatch.hpp>