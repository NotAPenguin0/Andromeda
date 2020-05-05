#ifndef ANDROMEDA_RENDERER_GEOMETRY_PASS_HPP_
#define ANDROMEDA_RENDERER_GEOMETRY_PASS_HPP_

#include <phobos/renderer/render_graph.hpp>
#include <andromeda/renderer/render_database.hpp>

#include <phobos/pipeline/shader_info.hpp>

namespace andromeda {
namespace renderer {

class GeometryPass {
public:
	GeometryPass(Context& ctx, ph::PresentManager& vk_present);

	void build(Context& ctx, ph::FrameInfo& frame, ph::RenderGraph& graph, RenderDatabase& database);

private:
	struct Bindings {
		ph::ShaderInfo::BindingInfo camera;
		ph::ShaderInfo::BindingInfo transforms;
		ph::ShaderInfo::BindingInfo textures;
	} bindings;

	ph::RenderAttachment* depth;
	ph::RenderAttachment* albedo_spec;
	ph::RenderAttachment* normal;

	struct PerFrameBuffers {
		ph::BufferSlice transforms;
		ph::BufferSlice camera_data;
	} per_frame_buffers;

	void create_pipeline(Context& ctx);

	void update_transforms(ph::CommandBuffer& cmd_buf, RenderDatabase& database);
	void update_camera_data(ph::CommandBuffer& cmd_buf, RenderDatabase& database);

	vk::DescriptorSet get_descriptors(ph::FrameInfo& frame, ph::CommandBuffer& cmd_buf, RenderDatabase& database);
};

}
}

#endif