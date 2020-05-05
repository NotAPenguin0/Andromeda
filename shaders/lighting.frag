#version 450

layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 0) uniform CameraData {
    mat4 projection_view;
    mat4 inverse_projection;
    mat4 inverse_view;
    vec3 position;
} camera;

struct PointLight {
    // First 3 values have the position, last has the radius of the light
    vec4 transform;
    // first 3 values are the color, last value has the intensity of the light
    vec4 color;
};

layout(set = 0, binding = 1) buffer readonly PointLights {
   PointLight lights[];
} lights;


layout(set = 0, binding = 2) uniform sampler2D gDepth;
layout(set = 0, binding = 3) uniform sampler2D gNormal;
layout(set = 0, binding = 4) uniform sampler2D gAlbedoSpec;

layout(push_constant) uniform PC {
    uint light_index;
    uvec2 screen_size;
} pc;

// adapted from https://stackoverflow.com/questions/32227283/getting-world-position-from-depth-buffer-value
// Note that we don't remap the depth value since the vk depth range goes from 0 to 1, not from -1 to 1 like in OpenGL
vec3 WorldPosFromDepth(float depth, vec2 TexCoords) {
    float z = depth;

    vec4 clipSpacePosition = vec4(TexCoords * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = camera.inverse_projection * clipSpacePosition;

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    vec4 worldSpacePosition = camera.inverse_view * viewSpacePosition;

    return worldSpacePosition.xyz;
}


float saturate(float x) {
    return max(0.0, min(x, 1.0));
}

vec3 apply_point_light(vec3 norm, vec3 in_color, PointLight light, vec3 WorldPos, float Specular) {
    vec3 light_dir = normalize(light.transform.xyz - WorldPos);
    float diff = max(dot(norm, light_dir), 0.0);
    vec3 diffuse = in_color * diff * light.color.xyz;

    vec3 view_dir = normalize(camera.position - WorldPos);
    vec3 reflect_dir = reflect(-light_dir, norm);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 2);
    vec3 specular = in_color * spec * light.color.xyz * Specular;

    float dist = length(WorldPos - light.transform.xyz);
    float radius = light.transform.w;
    float num = saturate(1 - pow(dist / radius, 4));
    float falloff = num * num / (dist * dist + 1);

    return (diffuse + specular) * falloff * light.color.w; // w component is intensity
}

vec2 CalculateGBufferTexCoords(uvec2 screen_size) {
    return vec2(gl_FragCoord.x / float(screen_size.x), gl_FragCoord.y / float(screen_size.y));
}

void main() {             
    vec2 GBufferTexCoords = CalculateGBufferTexCoords(pc.screen_size);
    // retrieve data from gbuffer
    vec3 Normal = texture(gNormal, GBufferTexCoords).rgb;
    vec4 AlbedoSpec = texture(gAlbedoSpec, GBufferTexCoords);
    vec3 Diffuse = AlbedoSpec.rgb;
    float Specular = AlbedoSpec.a;

    float depth = texture(gDepth, GBufferTexCoords).r;
    vec3 WorldPos = WorldPosFromDepth(depth, GBufferTexCoords);
    
    vec3 color = Diffuse;
    vec3 norm = Normal * 2.0 - 1.0;
    
    color = apply_point_light(norm, color, lights.lights[pc.light_index], WorldPos, Specular);

    FragColor = vec4(color, 1.0);
}

