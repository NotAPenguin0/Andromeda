#ifndef ANDROMEDA_SKYBOX_PASS_HPP_
#define ANDROMEDA_SKYBOX_PASS_HPP_

#include <phobos/renderer/render_graph.hpp>
#include <phobos/pipeline/shader_info.hpp>

#include <andromeda/renderer/render_database.hpp>

#include <andromeda/core/context.hpp>


namespace andromeda::renderer::deferred {

class SkyboxPass {
public:
	SkyboxPass(Context& ctx, ph::PresentManager& vk_present);

	struct Attachments {
		ph::RenderAttachment& output;
		ph::RenderAttachment& depth;
	};

	void build(Context& ctx, Attachments attachments, ph::FrameInfo& frame, ph::RenderGraph& graph, RenderDatabase& database);

private:
	struct Bindings {
		ph::ShaderInfo::BindingInfo transform;
		ph::ShaderInfo::BindingInfo skybox;
	} bindings;

	void create_pipeline(Context& ctx);
};

}

#endif