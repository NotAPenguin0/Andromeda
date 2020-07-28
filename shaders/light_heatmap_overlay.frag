#version 450

#define TILE_SIZE 16
#define MAX_LIGHTS_PER_TILE 1024

layout(location = 0) in vec2 UV;

layout(location = 0) out vec4 FragColor;

struct VisibleLightIndex {
	int index;
};

layout(set = 0, binding = 0) buffer readonly VisibleIndices {
	VisibleLightIndex data[];
} visible_light_indices;

layout(push_constant) uniform PC {
    uint tile_count_x;
} pc;


void main() {
	// Figure out the tile we're on and its index, so we can index into the visible indices buffer correctly
    const ivec2 pixel_position = ivec2(gl_FragCoord.xy);
    const ivec2 tile_id = pixel_position / ivec2(TILE_SIZE, TILE_SIZE);
    const uint tile_index = tile_id.y * pc.tile_count_x + tile_id.x;

    const uint light_index_offset = MAX_LIGHTS_PER_TILE * tile_index;
    uint light_count = 0;
    for(uint i = 0; (i < MAX_LIGHTS_PER_TILE) && (visible_light_indices.data[light_index_offset + i].index != -1); ++i) {
        ++light_count;
    }

    FragColor = vec4(float(light_count) / MAX_LIGHTS_PER_TILE, 0, 0, 1);
}