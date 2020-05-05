#ifndef ANDROMEDA_RENDERER_LIGHTING_PASS_HPP_
#define ANDROMEDA_RENDERER_LIGHTING_PASS_HPP_

#include <phobos/renderer/render_graph.hpp>
#include <phobos/pipeline/shader_info.hpp>

#include <andromeda/assets/mesh.hpp>
#include <andromeda/renderer/render_database.hpp>
#include <andromeda/util/handle.hpp>

namespace andromeda {
namespace renderer {

class LightingPass {
public:
	LightingPass(Context& ctx);

	struct Attachments {
		ph::RenderAttachment& output;
		ph::RenderAttachment& depth;
		ph::RenderAttachment& albedo_spec;
		ph::RenderAttachment& normal;
	};

	void build(Context& ctx, Attachments attachments, ph::FrameInfo& frame, ph::RenderGraph& graph, RenderDatabase& database);
private:
	struct PerFrameBuffers {
		ph::BufferSlice camera;
		ph::BufferSlice lights;
	} per_frame_buffers;

	struct Bindings {
		ph::ShaderInfo::BindingInfo depth;
		ph::ShaderInfo::BindingInfo albedo_spec;
		ph::ShaderInfo::BindingInfo normal;
		ph::ShaderInfo::BindingInfo camera;
		ph::ShaderInfo::BindingInfo lights;
	} bindings;

	Handle<Mesh> light_mesh_handle;

	void create_pipeline(Context& ctx);

	void create_light_mesh(Context& ctx);

	void update_camera_data(ph::CommandBuffer& cmd_buf, RenderDatabase& database);
	void update_lights(ph::CommandBuffer& cmd_buf, RenderDatabase& database);
	vk::DescriptorSet get_descriptors(ph::CommandBuffer& cmd_buf, ph::FrameInfo& frame, Attachments attachments);
};

}
}

#endif