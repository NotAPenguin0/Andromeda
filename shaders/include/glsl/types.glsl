// Note that this glsl file must be compatible with C++, as it is included in the rendering backend.

#ifdef __cplusplus

#include <glsl/limits.glsl>

#define vec4 glm::vec4
#define vec3 glm::vec3
#define mat4 glm::mat4

namespace gpu {

#else // GLSL
#include "limits.glsl"
#define alignas(x)
#endif

#ifdef __cplusplus
#pragma pack(push, 1)
#endif
struct PointLight {
    vec4 pos_radius;
    vec4 color_intensity;
    int shadow; // <0 if shadow caster, >=0 otherwise
    int _pad0[3]; // align to 16 bytes
};
#ifdef __cplusplus
#pragma pack(pop)
#endif

struct alignas(8 * sizeof(float)) DirectionalLight {
    // If w < 0, this directional light is not a shadow caster.
    vec4 direction_shadow;
    vec4 color_intensity;
};

#ifdef __cplusplus
#pragma pack(push, 1)
#endif
struct CascadeMapInfo {
    float splits[ANDROMEDA_SHADOW_CASCADE_COUNT];
    mat4 proj_view[ANDROMEDA_SHADOW_CASCADE_COUNT];
};
#ifdef __cplusplus
#pragma pack(pop)
#endif

#ifdef __cplusplus // C++, undefine and end namespace

#undef vec4
#undef vec3
#undef mat4
} // namespace gpu

#endif