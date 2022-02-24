#pragma once

#include <andromeda/graphics/backend/renderer_backend.hpp>

namespace andromeda::gfx::backend {

// Class exposing utility functions and storing resources of the atmospheric scattering rendering algorithm
// Implementation based on S. Hillaire, 2020 (https://sebh.github.io/publications/egsr2020.pdf)
class AtmosphereRendering {
public:
    struct LUTs {
        ph::ImageView transmittance;
        ph::ImageView sky_view;
    };

    explicit AtmosphereRendering(gfx::Context& ctx);
    ~AtmosphereRendering();

    ph::Pass lut_update_pass(ph::InFlightContext& ifc, gfx::SceneDescription const& scene);

    LUTs get_luts() const;
private:
    gfx::Context& ctx;

    ph::RawImage transmittance;
    ph::ImageView transmittance_view;
};

}