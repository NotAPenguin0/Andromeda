#pragma once

#include <string>
#include <vector>

namespace andromeda {

namespace str {

/**
 * @brief Split a string on a delimiter.
 * @param s String to split.
 * @param delim Delimiter character. Will not be included in the split string.
 * @return List of separately split strings. Will not include empty strings.
*/
std::vector<std::string> split(std::string const& s, char delim);

}

}