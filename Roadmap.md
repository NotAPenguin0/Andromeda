# Roadmap for development

## Main (rasterizer) rendering backend

- [X] Lighting components and material data
- [X] Forward+ rendering algorithm
  - [X] Initially use simple Blinn-Phong shading
  - [X] Debug displays
- [X] Environment maps
  - [X] Asset processing
  - [X] IBL
  - [X] Automatic exposure/iris adaptation/HDR scaling 
    - https://discord.com/channels/318590007881236480/600645448394342402/924972103692873749
    - https://docs.unrealengine.com/4.26/en-US/RenderingAndGraphics/PostProcessEffects/AutomaticExposure/
- [X] PBR shading model
- [ ] Shadows
  - [ ] Free up space used by shadowmaps that haven't been used in a long (?) time.
  - [ ] Configuration of settings
  - [ ] PCF/hardware PCF?
  - [ ] Soft shadows
- [ ] Organize backend code to be more re-usable.
  - [ ] Move postprocess pipeline to separate module
- [ ] GPU driven rendering
- [ ] Fog
- [ ] Water/fluid simulation
- [ ] Atmospheric scattering
- [ ] Vertex pulling instead of vertex attributes
- [ ] Put all meshes in a single large VkBuffer
- [ ] Terrain system
- [ ] Particles

## Asset pipeline

- [X] Make `assettool` take a single file as argument
- [X] Extend `assettool` with command line options
  - [X] RGB/sRGB color space
- [X] Make `assettool` work for images that have fewer channels.
- [X] Model loading and processing
  - [X] json model format linking multiple meshes and materials together
  - [X] processing `.obj` into `.ent` and necessary `.mesh` and `.tx` files
  - [X] add model loading to engine
    - [X] add blueprinting ECS
    - [X] add copying from blueprint ECS to real ECS.
    - [X] model loading creates a new blueprint entity
    - [X] loading model into scene copies the blueprint into the real ecs
- [ ] Asset hot-reloading

## UI/Editor

- [ ] Asset browser
  - [ ] Preview rendering
  - [ ] Drag and drop for asset handles
  - [ ] Integrate `assettool` into the main Andromeda project
- [ ] Gizmos
  - [ ] Rendering world grid
- [ ] Project system
  - [ ] Scene serialization
- [ ] Object picking in editor

## Small tweaks and patches

- [ ] Settings menu
- [ ] Performance display
- [ ] Color type for components fields to distinguish between vectors and colors
- [ ] Correctly handle tangent W, formula is `bitangent = cross(normal, tangent.xyz) * tangent.w`
  - Maybe assimp already handles this internally?
- [X] Support for materials that have solid color properties instead of textures
  - ex. mugs in coffee cart model
- [ ] Make `PostProcessingSettings` truly optional by removing stuff like automatic exposure from the pipeline if disabled.
- [ ] Fix case in renderer with no lights loaded
- [ ] Minimize allocations in the engine to further improve performance

## Farther future

- [ ] Raytraced rendering backend
  - Also add "hybrid" backend that only uses raytracing for specific tasks
    - This will most likely be the default backend.
  - Full raytraced backend should probably not be real-time and only update at certain intervals.
  