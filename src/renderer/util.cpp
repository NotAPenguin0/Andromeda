#include <andromeda/renderer/util.hpp>

#include <phobos/renderer/render_pass.hpp>

#include <stl/assert.hpp>

namespace andromeda::renderer {

// Adapted from ph::fixed::auto_viewport_scissor, but we need this here since we compile without the fixed pipeline
void auto_viewport_scissor(ph::CommandBuffer& cmd_buf) {
	ph::RenderPass* pass = cmd_buf.get_active_renderpass();
	STL_ASSERT(pass, "andromeda::renderer::auto_viewport called without an active renderpass");

	ph::RenderTarget const& target = pass->get_target();

	vk::Viewport viewport;
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = target.get_width();
	viewport.height = target.get_height();
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	cmd_buf.set_viewport(viewport);

	vk::Rect2D scissor;
	scissor.offset = vk::Offset2D{ 0, 0 };
	scissor.extent = vk::Extent2D{ target.get_width(), target.get_height() };
	cmd_buf.set_scissor(scissor);
}

}