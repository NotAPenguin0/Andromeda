#ifndef ANDROMEDA_TONEMAP_PASS_HPP_
#define ANDROMEDA_TONEMAP_PASS_HPP_

#include <phobos/pipeline/shader_info.hpp>
#include <andromeda/core/context.hpp>
#include <andromeda/renderer/render_database.hpp>

namespace andromeda {
namespace renderer {

class TonemapPass {
public:
	TonemapPass(Context& ctx);

	struct Attachments {
		ph::RenderAttachment& input_hdr;
		ph::RenderAttachment& output_ldr;
	};

	void build(Context& ctx, Attachments attachments, ph::FrameInfo& frame, ph::RenderGraph& graph, RenderDatabase& database);

private:
	struct Bindings {
		ph::ShaderInfo::BindingInfo input_hdr;
	} bindings;

	void create_pipeline(Context& ctx);
};

}
}

#endif