target_sources(andromeda PRIVATE 
	"main.cpp"

	"assets/assets.cpp"
	"assets/importers/importers.cpp"
	"assets/importers/stb_image.cpp"
	"assets/importers/png.cpp"

	"core/context.cpp"

	"ecs/registry.cpp"

	"renderer/geometry_pass.cpp"
	"renderer/renderer.cpp"
	"renderer/render_database.cpp"
	"renderer/util.cpp"

	"util/log.cpp"

	"world/world.cpp"

	"wsi/application.cpp"
	"wsi/window.cpp"
)