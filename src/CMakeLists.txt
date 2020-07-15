target_sources(andromeda PRIVATE 
	"main.cpp"

	"assets/assets.cpp"
	"assets/importers/importers.cpp"
	"assets/importers/stb_image.cpp"
	"assets/importers/png.cpp"

	"core/context.cpp"
	"core/task_manager.cpp"
	"core/env_map_loader.cpp"
	"core/mipmap_gen.cpp"

	"ecs/registry.cpp"

	"renderer/renderer.cpp"
	"renderer/render_database.cpp"
	"renderer/util.cpp"
	"renderer/deferred/deferred_renderer.cpp"
	"renderer/deferred/geometry_pass.cpp"
	"renderer/deferred/lighting_pass.cpp"
	"renderer/deferred/skybox_pass.cpp"
	"renderer/deferred/tonemap_pass.cpp"
	"renderer/forward_plus/forward_plus_renderer.cpp"

	"util/log.cpp"

	"world/world.cpp"

	"wsi/application.cpp"
	"wsi/window.cpp"
)