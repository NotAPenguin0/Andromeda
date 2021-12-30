#pragma once

#include <phobos/image.hpp>

#include <andromeda/util/handle.hpp>

namespace andromeda::gfx {

struct Environment {
    ph::RawImage cubemap;
    ph::ImageView cubemap_view;

    ph::RawImage irradiance;
    ph::ImageView irradiance_view;

    ph::RawImage specular;
    ph::ImageView specular_view;
};

}