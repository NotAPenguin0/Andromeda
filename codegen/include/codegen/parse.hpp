#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>

#include <cppast/libclang_parser.hpp>

namespace fs = std::filesystem;

// Stores information for a single field
struct FieldInfo {
	std::string name{};
	std::string type{};
};

// Stores information for a single component
struct ComponentInfo {
	// Name in (in code) of the component.
	std::string name{};
	// File name without directory (but including extension).
	std::string filename{};
	// Fields present in the component.
	std::vector<FieldInfo> fields;
};

// This structure stores a list of components that were extraced the from the parse operation.
struct ParseResult {
	std::vector<ComponentInfo> components;
};

struct Config {
	std::vector<std::string> include_directories;
};

void parse_file(ParseResult& result, fs::path path, Config const& config);
ParseResult parse_file(fs::path path, Config const& config);
