#include <andromeda/util/string_ops.hpp>

#include <sstream>

namespace andromeda::str {

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

}