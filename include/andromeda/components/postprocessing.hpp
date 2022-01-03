#pragma once

namespace andromeda {

/**
 * @brief PostProcessingSettings component. Stores various parameters and settings for postprocessing
 */
struct [[component]] PostProcessingSettings {
    [[editor::tooltip("Lower bound for luminance values in the scene. Lower values get clamped to this value. May not be zero.")]]
    [[editor::drag_speed(0.1)]]
    float min_log_luminance = -3.0f;

    [[editor::tooltip("Upper bound for luminance values in the scene. Higher values get clamped to this value. May not be zero.")]]
    [[editor::drag_speed(0.1)]]
    float max_log_luminance = 5.0f;
};

}