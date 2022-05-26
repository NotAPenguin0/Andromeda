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
    std::string tooltip{}; // Optional, empty string if not present.

    bool no_limits = false;

    // Stringified versions of editor values. Empty strings if not present.
    std::string min{};
    std::string max{};
    std::string drag_speed{};
    std::string format{};
};

// Stores information for a single component
struct ComponentInfo {
    // Name in (in code) of the component.
    std::string name{};
    // File name without directory (but including extension).
    std::string filename{};
    // Fields present in the component.
    std::vector<FieldInfo> fields;
    // Whether the editor::hide attribute was present.
    bool editor_hide = false;
};

// This structure stores a list of components that were extraced the from the parse operation.
struct ParseResult {
    std::vector<ComponentInfo> components;
    std::vector<std::string> unique_field_types;
};

struct Config {
    std::vector<std::string> include_directories;
};

void parse_file(ParseResult& result, fs::path path, Config const& config);

ParseResult parse_file(fs::path path, Config const& config);
