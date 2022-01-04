#pragma once

// This file exposes an interface to integrate Cascaded Shadow Mapping into a rendering pipeline.

#include <andromeda/graphics/backend/renderer_backend.hpp>

namespace andromeda::gfx::backend {

/**
 * @class CascadedShadowMapping
 * @brief This class manages resources used for CSM, and implements the creation of a shadow
 *        map creation pass.
 */
class CascadedShadowMapping {
public:
    /**
     * @brief Creates pipeline for shadowmap rendering.
     * @param ctx
     */
    static void create_pipeline(gfx::Context& ctx);


private:

};

}