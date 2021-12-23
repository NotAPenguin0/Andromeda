# Roadmap for development

## Main (rasterizer) rendering backend

- [X] Lighting components and material data
- [X] Forward+ rendering algorithm
  - [X] Initially use simple Blinn-Phong shading
  - [X] Debug displays
- [ ] Environment maps
  - [ ] Asset processing
  - [ ] IBL
- [X] PBR shading model

## Asset pipeline

- [X] Make `assettool` take a single file as argument
- [X] Extend `assettool` with command line options
  - [X] RGB/sRGB color space
- [X] Make `assettool` work for images that have fewer channels.
- [ ] Model loading and processing
  - [X] json model format linking multiple meshes and materials together
  - [ ] processing `.obj` into `.ent` and necessary `.mesh` and `.tx` files
  - [ ] add model loading to engine
    - [ ] add blueprinting ECS
    - [ ] add copying from blueprint ECS to real ECS.
    - [ ] model loading creates a new blueprint entity
    - [ ] loading model into scene copies the blueprint into the real ecs

## UI

- [ ] Asset browser
  - [ ] Preview rendering
  - [ ] Drag and drop for asset handles
  - [ ] Integrate `assettool` into the main Andromeda project

## Small tweaks and patches

- [ ] Settings menu
- [ ] Performance display
- [ ] Color type for components fields to distinguish between vectors and colors

## Farther future

- [ ] Project system
  - [ ] Scene serialization
- [ ] Raytraced rendering backend
  - Also add "hybrid" backend that only uses raytracing for specific tasks
  - Full raytraced backend should probably not be real-time and only update at certain intervals.
- [ ] Gizmos
- [ ] Water/fluid simulation