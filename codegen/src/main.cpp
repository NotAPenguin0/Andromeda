#include <iostream>
#include <fstream>

#include <sstream>

#include <codegen/parse.hpp>
#include <codegen/generate.hpp>

void print_parse_result(ParseResult const& res) {
	for (auto const& comp : res.components) {
		std::cout << "Component: " << comp.name << "\n";
		std::cout << "{\n";
		for (auto const& field : comp.fields) {
			std::cout << "\t" << field.type << " " << field.name << ";\n";
		}
		std::cout << "};\n";
		std::cout << "Defined in file: " << comp.filename << std::endl;
	}
}

std::vector<std::string> split(std::string const& s, char delim) {
	std::vector<std::string> result;
	std::stringstream ss(s);
	std::string item;

	while (std::getline(ss, item, delim)) {
		if (!item.empty())
			result.push_back(item);
	}

	return result;
}

int main(int argc, char** argv) {
	fs::path input_dir = fs::path(argv[1]);
	fs::path output_dir = fs::path(argv[2]);
	fs::path template_dir = fs::path(argv[3]);

	// Create directories for output files if they don't exist already.
	fs::create_directories(output_dir / "src");
	fs::create_directories(output_dir / "include");

	// Grab include directories from command-line arguments.
	Config parseconfig{};
	std::string include_dir_str = argv[4];
	// Split on ';', as this is how cmake splits the list elements
	parseconfig.include_directories = split(include_dir_str, ';');

	std::cout << "Include dirs:\n";
	for(auto const& include : parseconfig.include_directories) {
		std::cout << include << '\n';
	}
	std::cout << std::endl;

	ParseResult result;
	for (fs::directory_entry file : fs::directory_iterator(input_dir)) {
		parse_file(result, file, parseconfig);
	}

	generate_type_list_file(template_dir, output_dir, result);
	generate_reflect_decl_file(template_dir, output_dir, result);
	generate_reflection_source(template_dir, output_dir, result);

	return 0;
}