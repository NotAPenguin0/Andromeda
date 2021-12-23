# 0. Asset files

### a) Common traits and features

There are a few features that are shared between asset files. Generally most asset files will have a JSON section with metadata about the asset,
and optionally a binary blob with raw data.

### b) Compression

Typically, binary data will be compressed. The compression mode used will be stored in the JSON section.
The supported compression modes are:

- `None`: No compression used, simply `memcpy()` the data.
- `LZ4`: Standard LZ4 compression (see lz4 library in assetlib library).

Note that when loading the data will be automatically decompressed by the assetlib, so you don't need to manually do this.

# 1. Texture files (.tx)

### a) Identification

Texture files are binary files. They start with a 4-byte header identifying the file type. This header is `ITEX`.
The header is followed by a 32-bit integer describing the .tx version used.

The whole .tx file consists of two sections: a JSON section with metadata about the texture, and a binary section with raw (possibly compressed) texture data.
After the header are two 32-bit integers: the first is the length of the JSON string in bytes, the second is the length of the binary blob in bytes.

### b) JSON metadata.

The JSON section has a number of fields with metadata about the texture.

- `byte_size`: Size in bytes of the uncompressed texture. 
- `compression_mode`: Compression mode used by the asset packer. For more information see earlier in this document.
- `extents`:
    - `x`: Width of the texture in pixels
    - `y`: Height of the texture in pixels
- `format`: Texture format. This is a string describing the pixel format. For more information see (c) Pixel formats.
- `mip_levels`: Amount of mip levels stored in the binary blob.
- `color_space`: Color space of the texture. This is optional and will default to `RGB` if not present. Possible values are:
  - `RGB`
  - `sRGB`

### c) Pixel formats

Currently, the following pixel formats are supported:
	
- `R8`: 8 bits per component, 1 component.
- `RG8`: 8 bits per component, 2 components.
- `RGB8`: 8 bits per component, 3 components. Note that not many GPUs support this formay for images.
- `RGBA8`: 8 bits per component, 4 components.

### d) Binary blob

The (compressed) binary blob stores raw pixel data for the image. Mip levels are stored starting at level 0, packed tightly next to each other.

 # 2. Mesh files (.mesh)		 

###a) Identification

Like texture files, mesh files are binary files. They also have a 4-byte header identifying the file type, which is `IMESH`.
The next 4 bytes hold a 32-bit integer describing the .mesh version used.

The entire .mesh file, just like texture files, consists of two sections: a JSON header and a binary blob.

### b) JSON metadata

The fields in the JSON section for textures are the following:

- `compression_mode`: Compression mode used by the asset packer.
- `index_binary_offset`: Offset in the binary blob where the index data starts.
- `index_bits`: Amount of bits used for each index. Possible values are 32 and 16 (however right now, only 32 bit indices are supported).
- `index_count`: Amount of indices the mesh has.
- `vertex_count`: Amount of vertices the mesh has.
- `vertex_format`: The vertex format that was used. These have special names that can be easily translated into a format. For more information see *(c) Vertex formats*.

### c) Vertex formats
	
Every letter in the vertex format string corresponds to an attribute. Currently, supported are:

- `P`: Position vector (3 components).
- `N`: Normal vector (3 components).
- `T`: Tangent vector (3 components).
- `V`: UV values (texture coordinates, 2 components).

Currently, supported attributes are:

- `PNTV32`: Position/Normal/Tangent/UV, all values are 32-bit floats.

### d) Binary blob

The binary blob stores both vertex and index data. First is the vertex data, and then at offset `index_binary_offset` the index data starts.

 # 3. Material files (.mat)		 

### a) Identification

Unlike the previous file types, material files are not binary files. They are simple JSON files describing the material's properties.
Currently, the following properties are always present:

- `albedo`: path to the albedo texture. For more information on paths, see *(b) Relative paths*.

Additionally, there are a number of optional properties to further refine a material and make a full PBR-compatible material.

- `normal`: path to the normal map texture.
- `metal_rough`: path to the metallic/roughness texture.
- `occlusion`: path to an ambient occlusion map texture.

### b) Relative paths

When a relative path is encountered, it will be seen as relative to the root of the asset system. Currently, (with no project files existing) this is
the executable folder. Once project files are added this will be the `assets` folder in the project directory.

# 4. Entity files (.ent)

### a) Identification

Entity files, just like material files, are JSON files. They contain a single JSON object representing the entity and its components,
as well as a list of child entities (each defining components and children). `.ent` files are the result of
processing a model using `assettool` or of exporting an entity or scene.

### b) Components

Valid components and their attributes are listed below:

- `MeshRenderer`: Holds material and other rendering information for this entity.
  - `material`: Path to the relevant `.mtl` file relative to **the assets folder**.
  - `mesh`: Path to the `.mesh` file of the mesh to be rendered.
- `Name`: Stores a string name for this entity
  - `name`: String with the name of the entity
- `Transform`: Transformation info for the entity.
  - `position`: List of 3 values containing the position relative to its parent
  - `rotation`: List of 3 values describing the rotation (in degrees) relative to its parent.
  - `scale`: List of 3 values describing the scale factor relative to its parent's scale.

**Important note: All paths inside the entity components are relative the main assets folder, not to their own file.**