#pragma once

#include <codegen/parse.hpp>

std::string read_file(fs::path path);

void generate_type_list_file(fs::path template_dir, fs::path output_dir, ParseResult const& data);
void generate_reflect_decl_file(fs::path template_dir, fs::path output_dir, ParseResult const& data);

void generate_reflection_source(fs::path template_dir, fs::path output_dir, ParseResult const& data);