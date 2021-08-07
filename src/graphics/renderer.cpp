#include <andromeda/graphics/renderer.hpp>

#include <phobos/render_graph.hpp>

namespace andromeda{
namespace gfx {

Renderer::Renderer(gfx::Context& ctx) {

}

void Renderer::render_frame(gfx::Context& ctx, World const& world) {
	ph::InFlightContext ifc = ctx.wait_for_frame();
	
	ph::Pass clear = ph::PassBuilder::create("clear")
		.add_attachment(ctx.get_swapchain_attachment_name(), 
						ph::LoadOp::Clear, { .color = {1.0f, 0.0f, 0.0f, 1.0f} })
		.get();

	ph::RenderGraph graph{};
	graph.add_pass(std::move(clear));

	graph.build(ctx);

	ph::RenderGraphExecutor executor{};
	ifc.command_buffer.begin();
	executor.execute(ifc.command_buffer, graph);
	ifc.command_buffer.end();

	ctx.submit_frame_commands(*ctx.get_queue(ph::QueueType::Graphics), ifc.command_buffer);
	ctx.present(*ctx.get_present_queue());
}

} // namespace gfx
} // namespace andromeda