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

    AtmosphereRendering(gfx::Context& ctx, VkSampleCountFlagBits samples, float sample_ratio);
    ~AtmosphereRendering();

    ph::Pass lut_update_pass(gfx::Viewport const& vp, ph::InFlightContext& ifc, gfx::SceneDescription const& scene);

    void render_atmosphere(gfx::Viewport const& vp, ph::CommandBuffer& cmd, ph::InFlightContext& ifc, ph::BufferSlice camera, gfx::SceneDescription const& scene);

    [[nodiscard]] LUTs get_luts() const;

private:
    gfx::Context& ctx;

    ph::RawImage transmittance;
    ph::ImageView transmittance_view;

    ph::RawImage sky;
    ph::ImageView sky_view;
};

}