target_sources(andromeda PRIVATE 
	"main.cpp"
	
	"app/application.cpp"
	"app/log.cpp"
	"app/wsi.cpp"

	"assets/assets.cpp"
	"assets/texture_loader.cpp"

	"ecs/registry.cpp"

	"graphics/context.cpp"
	"graphics/renderer.cpp"

	"thread/scheduler.cpp"

	"world.cpp"
)