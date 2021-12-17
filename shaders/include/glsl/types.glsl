// Note that this glsl file must be compatible with C++, as it is included in the rendering backend.

#ifdef __cplusplus

#define vec4 glm::vec4
#define vec3 glm::vec3
namespace gpu {

#else // GLSL
#define alignas(x)
#endif

struct alignas(8 * sizeof(float)) PointLight {
    vec4 pos_radius;
    vec4 color_intensity;
};

#ifdef __cplusplus // C++, undefine and end namespace

#undef vec4
#undef vec3
} // namespace gpu

#endif