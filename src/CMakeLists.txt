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

	"renderer/geometry_pass.cpp"
	"renderer/renderer.cpp"
	"renderer/render_database.cpp"
	"renderer/lighting_pass.cpp"
	"renderer/util.cpp"
	"renderer/skybox_pass.cpp"
	"renderer/tonemap_pass.cpp"

	"util/log.cpp"

	"world/world.cpp"

	"wsi/application.cpp"
	"wsi/window.cpp"
)