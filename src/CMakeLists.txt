target_sources(andromeda PRIVATE 
	"main.cpp"
	
	"app/application.cpp"
	"app/log.cpp"
	"app/wsi.cpp"

	"assets/assets.cpp"
	"assets/mesh_loader.cpp"
	"assets/texture_loader.cpp"

	"ecs/registry.cpp"

	"graphics/context.cpp"
	"graphics/renderer.cpp"
	"graphics/imgui_impl.cpp"
	"graphics/imgui_impl_glfw.cpp"

	"thread/scheduler.cpp"

	"world.cpp"
)