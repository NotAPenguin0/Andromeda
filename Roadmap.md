# Roadmap for development

## Main (rasterizer) rendering backend

- Full model loading
    - Blueprint system
- Lighting components and material data
- Forward+ rendering algorithm
  - Initially use simple Blinn-Phong shading
  - Debug displays
- Environment maps
- PBR shading model

## Asset pipeline

- Make `assettool` take a single file as argument
- Extend `assettool` with command line options
  - RGB/sRGB color space
- Make `assettool` work for images that have fewer channels.

## UI

- Asset browser
  - Preview rendering
  - Drag and drop for asset handles
  - Integrate `assettool` into the main Andromeda project

## Small tweaks and patches

- Settings menu
- Performance display
- Color type for components fields to distinguish between vectors and colors

## Farther future

- Project system
  - Scene serialization
- Raytraced rendering backend
  - Also add "hybrid" backend that only uses raytracing for specific tasks
- Gizmos
- Water/fluid simulation