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
- [ ] Organize backend code to be more re-usable.
  - [ ] Move postprocess pipeline to separate module

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

## UI

- [ ] Asset browser
  - [ ] Preview rendering
  - [ ] Drag and drop for asset handles
  - [ ] Integrate `assettool` into the main Andromeda project

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

## Farther future

- [ ] Project system
  - [ ] Scene serialization
- [ ] Raytraced rendering backend
  - Also add "hybrid" backend that only uses raytracing for specific tasks
  - Full raytraced backend should probably not be real-time and only update at certain intervals.
- [ ] Gizmos
  - [ ] Rendering world grid
- [ ] Water/fluid simulation
- [ ] Object picking in editor
- [ ] Atmospheric scattering
- [ ] Asset hot-reloading
- [ ] GPU driven rendering
- [ ] Fog