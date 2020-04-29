target_sources(andromeda PRIVATE 
	"main.cpp"

	"assets/assets.cpp"
	"assets/importers/importers.cpp"
	"assets/importers/png.cpp"

	"core/context.cpp"

	"ecs/registry.cpp"

	"renderer/renderer.cpp"

	"util/log.cpp"

	"world/world.cpp"

	"wsi/application.cpp"
	"wsi/window.cpp"
)