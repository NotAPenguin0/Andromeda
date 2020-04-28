#ifndef ANDROMEDA_VERSION_HPP_
#define ANDROMEDA_VERSION_HPP_

#include <andromeda/util/types.hpp>

namespace andromeda {

struct Version {
	size_t major, minor, patch;
};

}

#define ANDROMEDA_VERSION ::andromeda::Version{0, 0, 1};

#endif