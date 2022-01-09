#include <codegen/parse.hpp>

#include <cppast/cpp_member_variable.hpp>
#include <cppast/visitor.hpp>
#include <cppast/cpp_entity_kind.hpp>

#include <iostream>

static bool ent_has_attribute(cppast::cpp_entity const& entity, std::string const& a) {
    return cppast::has_attribute(entity, a).has_value();
}

// Note that we assume a single component per file.
struct ast_visitor {
    // Extracted information from an ast.
    ComponentInfo meta{};
    // Whether a component was found in this file.
    bool found_component = false;

    bool operator()(cppast::cpp_entity const& entity, cppast::visitor_info visit_info) {

        // For a file, we want to extrac the name of the file so we can use this to include the component.
        if (entity.kind() == cppast::cpp_entity_kind::file_t) {
            fs::path path = fs::path(entity.name());
            meta.filename = path.filename().generic_string();
        }

        // On entering a container we need to set depth information correctly.
        if (visit_info.event == cppast::visitor_info::container_entity_enter) {
            depth += 1;
            // If we are not inside a component, and the current entity is a component then we now are inside one.
            if (!inside_component && is_component(entity)) {
                found_component = true;
                inside_component = true;
                component_depth = depth;

                meta.name = entity.name();

                meta.editor_hide = ent_has_attribute(entity, "editor::hide");
            }
        }

            // On exiting a container we need to update depth information again
        else if (visit_info.event == cppast::visitor_info::container_entity_exit) {
            depth -= 1;
            // We just exit the current component
            if (depth < component_depth) {
                component_depth = 0;
                inside_component = false;
            }
        }

            // While we are inside a component, look for fields to process
        else if (is_field(entity, visit_info)) {
            parse_field(entity);
        }

        return true;
    }

private:
    // How deep we are in eventual nested structures.
    int depth = 0;
    // Depth of the component we are processing
    int component_depth = 0;
    // Whether we are currently inside a component/
    bool inside_component = false;

    bool is_component(cppast::cpp_entity const& entity) {
        return (entity.kind() == cppast::cpp_entity_kind::class_t) && ent_has_attribute(entity, "component");
    }

    bool is_field(cppast::cpp_entity const& entity, cppast::visitor_info const& info) {
        return entity.kind() == cppast::cpp_entity_kind::member_variable_t
               && info.access == cppast::cpp_access_specifier_kind::cpp_public;
    }

    // Parse field into the resulting meta structure.
    void parse_field(cppast::cpp_entity const& entity) {
        FieldInfo& field = meta.fields.emplace_back();
        auto const& field_data = static_cast<cppast::cpp_member_variable const&>(entity);
        field.name = field_data.name();
        field.type = cppast::to_string(field_data.type());

        if (ent_has_attribute(entity, "editor::tooltip")) {
            cppast::cpp_attribute const& attribute = cppast::has_attribute(entity, "editor::tooltip").value();
            if (auto args = attribute.arguments(); args.has_value() && !args.value().empty()) {
                // Note that this stil includes the quotes, so we need to remove those.
                std::string tooltip_str = args.value().front().spelling;
                // Empty tooltip string.
                if (tooltip_str.size() == 2) { return; }

                // remove the quotes.
                field.tooltip = tooltip_str.substr(1, tooltip_str.size() - 2);
            } else {
                std::cout << "Parse error in component " << meta.name << " field " << field.name << ": "
                          << "editor::tooltip(): expected one argument";
            }
        }

        if (ent_has_attribute(entity, "editor::min")) {
            cppast::cpp_attribute const& attribute = cppast::has_attribute(entity, "editor::min").value();
            if (auto args = attribute.arguments(); args.has_value() && !args.value().empty()) {
                field.min = args.value().front().spelling;
            } else {
                std::cout << "Parse error in component " << meta.name << " field " << field.name << ": "
                          << "editor::min(): expected one argument";
            }
        }

        if (ent_has_attribute(entity, "editor::max")) {
            cppast::cpp_attribute const& attribute = cppast::has_attribute(entity, "editor::max").value();
            if (auto args = attribute.arguments(); args.has_value() && !args.value().empty()) {
                field.max = args.value().front().spelling;
            } else {
                std::cout << "Parse error in component " << meta.name << " field " << field.name << ": "
                          << "editor::max(): expected one argument";
            }
        }

        if (ent_has_attribute(entity, "editor::drag_speed")) {
            cppast::cpp_attribute const& attribute = cppast::has_attribute(entity, "editor::drag_speed").value();
            if (auto args = attribute.arguments(); args.has_value() && !args.value().empty()) {
                field.drag_speed = args.value().front().spelling;
            } else {
                std::cout << "Parse error in component " << meta.name << " field " << field.name << ": "
                          << "editor::drag_speed(): expected one argument";
            }
        }
    }
};

static void parse_file_into_result(ParseResult& result, std::unique_ptr<cppast::cpp_file> const& file) {
    ast_visitor visitor{};
    cppast::visit(*file, [&visitor](cppast::cpp_entity const& e, cppast::visitor_info info) -> bool {
        return visitor(e, info);
    });
    if (visitor.found_component) {
        result.components.push_back(visitor.meta);
    }
}

void parse_file(ParseResult& result, fs::path path, Config const& configuration) {
    // Setup cppast configuration.
    cppast::libclang_compile_config config;
    cppast::compile_flags flags{};
    flags |= cppast::compile_flag::ms_compatibility;

    config.set_flags(cppast::cpp_standard::cpp_20, flags);

    // Set include directories based on configuration
    for (auto const& dir: configuration.include_directories) {
        config.add_include_dir(dir);
    }

    // We want to log our errors to stderr so they show up as actual errors in the IDE.
    cppast::stderr_diagnostic_logger logger{};
    cppast::libclang_parser parser{type_safe::ref(logger)};
    cppast::cpp_entity_index idx{};
    std::cout << "Parsing component file: " << path.generic_string() << std::endl;
    auto file = parser.parse(idx, path.generic_string(), config);
    if (parser.error()) {
        std::cout << "Parser error occured\n";
    }

    parse_file_into_result(result, file);
}

ParseResult parse_file(fs::path path, Config const& configuration) {
    ParseResult result;
    parse_file(result, path, configuration);
    return result;
}