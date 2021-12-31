// Note that this glsl file must be compatible with C++, as it is included in the rendering backend.

#define ANDROMEDA_MAX_LIGHTS_PER_TILE 1024
#define ANDROMEDA_TILE_SIZE 16

// Parameters for automatic exposure algorithm
#define ANDROMEDA_LUMINANCE_BINS 256
#define ANDROMEDA_LUMINANCE_ACCUMULATE_GROUP_SIZE 16