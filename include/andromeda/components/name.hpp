#pragma once

#include <string>

namespace andromeda {

/**
 * @brief Name component
*/
struct [[component, editor::hide]] Name {
    std::string name;
};

}