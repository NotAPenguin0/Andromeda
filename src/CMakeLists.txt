target_sources(andromeda PRIVATE
        "main.cpp"

        "app/application.cpp"
        "app/log.cpp"
        "app/wsi.cpp"

        "assets/assets.cpp"
        "assets/entity_loader.cpp"
        "assets/environment_loader.cpp"
        "assets/material_loader.cpp"
        "assets/mesh_loader.cpp"
        "assets/texture_loader.cpp"

        "ecs/registry.cpp"

        "editor/command_parser.cpp"
        "editor/console.cpp"
        "editor/editor.cpp"
        "editor/inspector.cpp"
        "editor/performance.cpp"
        "editor/scene_viewport.cpp"
        "editor/style.cpp"
        "editor/widgets/dockspace.cpp"
        "editor/widgets/floating_panel.cpp"
        "editor/widgets/image.cpp"
        "editor/widgets/input_text.cpp"
        "editor/widgets/menu_bar.cpp"
        "editor/widgets/table.cpp"
        "editor/widgets/tooltip.cpp"

        "graphics/backend/csm.cpp"
        "graphics/backend/debug_geometry.cpp"
        "graphics/backend/depth_pass.cpp"
        "graphics/backend/forward_plus.cpp"
        "graphics/backend/mesh_draw.cpp"
        "graphics/backend/rtx.cpp"
        "graphics/backend/skybox.cpp"
        "graphics/backend/tonemap.cpp"
        "graphics/context.cpp"
        "graphics/imgui_impl.cpp"
        "graphics/imgui_impl_glfw.cpp"
        "graphics/performance_counters.cpp"
        "graphics/renderer.cpp"
        "graphics/scene_description.cpp"

        "math/transform.cpp"

        "thread/scheduler.cpp"

        "util/string_ops.cpp"

        "world.cpp"
        )