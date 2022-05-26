#include <codegen/generate.hpp>

#include <fstream>
#include <set>

#include <mustache.hpp>

using namespace kainjow;

std::string read_file(fs::path path) {
    std::ifstream file(path);
    if (!file) { return ""; }
    return std::string{std::istreambuf_iterator<char>{file}, {}};
}

void create_file(fs::path path) {
    std::ofstream file(path);
}

void output(fs::path const& dir, std::string const& filename, std::string const& content) {
    create_file(dir / filename);
    std::ofstream file(dir / filename, std::ostream::trunc);
    file << content;

    std::cout << "Writing output to file: " << (dir / filename).generic_string() << std::endl;
}

void generate_type_list_file(fs::path template_dir, fs::path output_dir, ParseResult const& data) {
    std::string tpl = read_file(template_dir / "type_lists_hpp.tpl");
    mustache::mustache must{tpl};

    mustache::data must_data{mustache::data::type::object};

    mustache::data& types_data = must_data["field_types"] = mustache::data::type::list;
    mustache::data& includes = must_data["includes"] = mustache::data::type::list;
    mustache::data& components_data = must_data["component_types"] = mustache::data::type::list;

    for (auto const& comp: data.components) {
        includes << mustache::data{"filename", comp.filename};
    }

    for (int i = 0; i < data.unique_field_types.size(); ++i) {
        mustache::data type_data{};
        type_data["type"] = data.unique_field_types[i];
        type_data["id"] = std::to_string(i);

        types_data << type_data;
    }

    for (auto it = data.components.begin(); it != data.components.end(); ++it) {
        mustache::data dat{};
        dat["type"] = it->name;
        if (it != data.components.end() - 1) { dat["comma"] = ","; }
        else { dat["comma"] = ""; }

        components_data << dat;
    }

    std::string generated = must.render(must_data);
    output(output_dir, "include/reflect/type_lists.hpp", generated);
}

void generate_reflect_decl_file(fs::path template_dir, fs::path output_dir, ParseResult const& data) {
    std::string tpl = read_file(template_dir / "component_reflect_decl_hpp.tpl");
    mustache::mustache must{tpl};
    mustache::data must_data{mustache::data::type::object};
    mustache::data& decl_list = must_data["decl"] = mustache::data::type::list;

    for (auto const& comp: data.components) {
        decl_list << mustache::data{"component", comp.name};
    }

    output(output_dir, "include/reflect/component_reflect_decl.hpp", must.render(must_data));
}

static uint32_t find_type_id(ParseResult const& data, std::string const& type) {
    for (uint32_t i = 0; i < data.unique_field_types.size(); ++i) {
        if (type == data.unique_field_types[i]) { return i; }
    }

    throw std::runtime_error("Field type not found");
}

static std::string max_value_string(std::string const& type) {
    if (type == "float") { return std::to_string(std::numeric_limits<float>::max()); }
    else { return "{}"; }
}

void generate_reflection_source(fs::path template_dir, fs::path output_dir, ParseResult const& data) {
    std::string tpl = read_file(template_dir / "reflection_cpp.tpl");
    mustache::mustache must{tpl};
    mustache::data must_data{mustache::data::type::object};

    mustache::data& impl_list = must_data["impl"] = mustache::data::type::list;
    for (auto const& comp: data.components) {
        mustache::data impl_data{};
        impl_data["component"] = comp.name;
        mustache::data& field_list = impl_data["fields"] = mustache::data::type::list;

        for (auto it = comp.fields.begin(); it != comp.fields.end(); ++it) {
            mustache::data field_data{};
            field_data["type"] = it->type;
            field_data["name"] = it->name;
//			field_data["type_id"] = std::to_string(find_type_id(data, it->type));
            field_data["tooltip"] = it->tooltip;

            std::vector<std::string> field_flags{};
            field_flags.push_back("none");
            if (it->min.empty() && it->max.empty()) { field_flags.push_back("no_limits"); }
            else if (it->no_limits) {field_flags.push_back("no_limits"); }

            // Default construct
            if (it->min.empty()) { field_data["min"] = "{}"; }
            else { field_data["min"] = it->min; }

            if (it->max.empty()) { field_data["max"] = max_value_string(it->type); }
            else { field_data["max"] = it->max; }

            if (it->drag_speed.empty()) { field_data["drag_speed"] = "1.0f"; }
            else { field_data["drag_speed"] = it->drag_speed; }

            field_data["format"] = it->format;
            if (it->format.empty()) {
                if (it->type == "glm::vec2" || it->type == "glm::vec3" || it->type == "glm::vec4" || it->type == "float") {
                    field_data["format"] = "\"%.3f\"";
                } else {
                    field_data["format"] = "\"\"";
                }
            }


            if (it != comp.fields.end() - 1) { field_data["comma"] = ","; }
            else { field_data["comma"] = ""; }


            mustache::data& flags_list = field_data["flags"] = mustache::data::type::list;

            for (int i = 0; i < field_flags.size(); ++i) {
                mustache::data flag_data{};
                flag_data["flag"] = field_flags[i];
                if (i != field_flags.size() - 1) { flag_data["or"] = "|"; }
                else { flag_data["or"] = ""; }

                flags_list << flag_data;
            }

            field_list << field_data;
        }

        mustache::data& flags_list = impl_data["flags"] = mustache::data::type::list;
        std::vector<std::string> flags_str{};

        // Create type flags, we only add none to make sure the argument is not empty.
        flags_str.push_back("none");
        if (comp.editor_hide) {
            flags_str.push_back("editor_hide");
        }

        for (int i = 0; i < flags_str.size(); ++i) {
            mustache::data flag_data{};
            flag_data["flag"] = flags_str[i];
            if (i != flags_str.size() - 1) { flag_data["or"] = "|"; }
            else { flag_data["or"] = ""; }

            flags_list << flag_data;
        }

        impl_list << impl_data;
    }

    output(output_dir, "src/reflection.cpp", must.render(must_data));
}

void generate_dispatch(fs::path template_dir, fs::path output_dir, ParseResult const& data) {
    std::string tpl = read_file(template_dir / "dispatch_hpp.tpl");
    mustache::mustache must{tpl};
    mustache::data must_data{mustache::data::type::object};

    mustache::data& types_list = must_data["type_list"] = mustache::data::type::list;
    for (uint32_t i = 0; i < data.unique_field_types.size(); ++i) {
        mustache::data type;
        type["id"] = std::to_string(i);
        type["type"] = data.unique_field_types[i];

        types_list << type;
    }

    output(output_dir, "include/reflect/dispatch.hpp", must.render(must_data));
}