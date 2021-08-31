#include <codegen/generate.hpp>

#include <fstream>
#include <set>

#include <mustache.hpp>
using namespace kainjow;

std::string read_file(fs::path path) {
	std::ifstream file(path);
	if (!file) return "";
	return std::string{ std::istreambuf_iterator<char>{file}, {} };
}

void create_file(fs::path path) {
	std::ofstream file(path);
}

void output(fs::path const& dir, std::string const& filename, std::string const& content) {
	create_file(dir / filename);
	std::ofstream file(dir / filename, std::ostream::trunc);
	file << content;
}

void generate_type_list_file(fs::path template_dir, fs::path output_dir, ParseResult const& data) {
	std::string tpl = read_file(template_dir / "type_lists_hpp.tpl");
	mustache::mustache must{ tpl };

	mustache::data must_data{mustache::data::type::object};
	
	mustache::data& types_data = must_data["field_types"] = mustache::data::type::list;
	mustache::data& includes = must_data["includes"] = mustache::data::type::list;
	mustache::data& components_data = must_data["component_types"] = mustache::data::type::list;

	// Grab all field types found in the component list, also add all include files while we're at it.
	std::set<std::string> unique_field_types{};
	for (auto const& comp : data.components) {
		for (auto const& field : comp.fields) {
			unique_field_types.insert(field.type);
		}

		includes << mustache::data{ "filename", comp.filename };
	}
	// flatten to a vector, we need this to remove the comma at the end
	std::vector<std::string> types(unique_field_types.begin(), unique_field_types.end());

	for (auto it = types.begin(); it != types.end(); ++it) {
		mustache::data type_data{};
		type_data["type"] = *it;
		if (it != types.end() - 1) type_data["comma"] = ",";
		else type_data["comma"] = "";

		types_data << type_data;
	}

	for (auto it = data.components.begin(); it != data.components.end(); ++it) {
		mustache::data dat{};
		dat["type"] = it->name;
		if (it != data.components.end() - 1) dat["comma"] = ",";
		else dat["comma"] = "";

		components_data << dat;
	}

	std::string generated = must.render(must_data);
	output(output_dir, "include/reflect/type_lists.hpp", generated);
}

void generate_reflect_decl_file(fs::path template_dir, fs::path output_dir, ParseResult const& data) {
	std::string tpl = read_file(template_dir / "component_reflect_decl_hpp.tpl");
	mustache::mustache must{ tpl };
	mustache::data must_data{ mustache::data::type::object };
	mustache::data& decl_list = must_data["decl"] = mustache::data::type::list;

	for (auto const& comp : data.components) {
		decl_list << mustache::data{"component", comp.name};
	}

	output(output_dir, "include/reflect/component_reflect_decl.hpp", must.render(must_data));
}

void generate_reflection_source(fs::path template_dir, fs::path output_dir, ParseResult const& data) {
	std::string tpl = read_file(template_dir / "reflection_cpp.tpl");
	mustache::mustache must{ tpl };
	mustache::data must_data{ mustache::data::type::object };

	mustache::data& impl_list = must_data["impl"] = mustache::data::type::list;
	for (auto const& comp : data.components) {
		mustache::data impl_data{};
		impl_data["component"] = comp.name;
		mustache::data& field_list = impl_data["fields"] = mustache::data::type::list;

		for (auto it = comp.fields.begin(); it != comp.fields.end(); ++it) {
			mustache::data field_data{};
			field_data["type"] = it->type;
			field_data["name"] = it->name;

			if (it != comp.fields.end() - 1) field_data["comma"] = ",";
			else field_data["comma"] = "";

			field_list << field_data;
		}

		impl_list << impl_data;
	}

	output(output_dir, "src/reflection.cpp", must.render(must_data));
}