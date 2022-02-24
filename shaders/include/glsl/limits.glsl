// Note that this glsl file must be compatible with C++, as it is included in the rendering backend.

#define ANDROMEDA_MAX_LIGHTS_PER_TILE 1024
#define ANDROMEDA_TILE_SIZE 16

// Parameters for automatic exposure algorithm
#define ANDROMEDA_LUMINANCE_BINS 256
#define ANDROMEDA_LUMINANCE_ACCUMULATE_GROUP_SIZE 16

#define ANDROMEDA_SHADOW_RESOLUTION 2048
#define ANDROMEDA_MAX_SHADOWING_DIRECTIONAL_LIGHTS 8
#define ANDROMEDA_SHADOW_CASCADE_COUNT 4

// Since we use simple temporal denoise we don't need a lot of rays
#define ANDROMEDA_SHADOW_RAYS 4

#define ANDROMEDA_TRANSMITTANCE_LUT_WIDTH 256
#define ANDROMEDA_TRANSMITTANCE_LUT_HEIGHT 64