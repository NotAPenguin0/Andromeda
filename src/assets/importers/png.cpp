#include <andromeda/assets/importers/png.hpp>

#include <stdio.h>

#include <unordered_map>

namespace andromeda {
namespace assets {
namespace importers {
namespace png {

// TODO: Proper error handling

struct InternalLoadData {
	uint8_t bit_depth = 0;
};

enum class ChunkType {
	// These chunks are required to be present in a valid PNG file
	IHDR, // Header
	PLTE, // Palette
	IDAT, // Data
	IEND, // End
	sRGB, // srgb information
	gAma, // gamma information
	pHYs, // Intended pixel size/aspect ratio. Ignored when parsing
	Invalid // Invalid or unsupported chunk type
};

struct ChunkInfo {
	ChunkType type = ChunkType::Invalid;
	uint32_t size = 0;

	// Amount of bytes the ChunkInfo struct takes up in a file
	static constexpr size_t file_bytes = 8;
};

struct ChunkHeader {
	static constexpr size_t size = 4;
	const byte data[4];
};

static std::unordered_map<ChunkType, ChunkHeader> chunk_headers{
	{ ChunkType::IHDR, { .data = { 73, 72, 68, 82 }} },
	{ ChunkType::sRGB, { .data = { 115, 82, 71, 66 }} },
	{ ChunkType::gAma, { .data = { 103, 65, 77, 65 }} },
	{ ChunkType::pHYs, { .data = { 112, 72, 89, 115 }} },
	{ ChunkType::IDAT, { .data = { 73, 68, 65, 84 }} }
};

static uint32_t to_big_endian(byte const* val) {
	return (uint32_t)val[3] | ((uint32_t)val[2] << 8) | ((uint32_t)val[1] << 16) | ((uint32_t)val[0] << 24);
}

// Read a single byte
static bool read_byte(FILE* file, byte& val) {
	if (fread(&val, 1, 1, file) < 1) {
		return false;
	}
	return true;
}

// Ignores count bytes from the file
static bool ignore_bytes(FILE* file, size_t count) {
	byte ignored;
	for (size_t i = 0; i < count; ++i) {
		if (!read_byte(file, ignored)) { return false; }
	}
	return true;
}

// Reads a u32 from a FILE* and converts it to big endian (since that's what PNG uses)
static bool read_u32(FILE* file, uint32_t& val) {
	byte buffer[sizeof(uint32_t)];
	if (fread(buffer, 1, sizeof(uint32_t), file) < sizeof(uint32_t)) {
		return false;
	}
	val = to_big_endian(buffer);
	return true;
}

static void check_signature(OpenTexture& texture, FILE* file) {
	// From the PNG spec (https://www.w3.org/TR/2003/REC-PNG-20031110/#5PNG-file-signature)
	// The first eight bytes of a PNG datastream always contain the following (decimal) values:
	//  137 80 78 71 13 10 26 10
	constexpr byte signature[]{ 137, 80, 78, 71, 13, 10, 26, 10 };
	constexpr size_t signature_size = sizeof(signature); // dividing by sizeof(byte) would be a bit silly
	byte buffer[signature_size];
	// Attempt to read, and if not enough bytes were read, return an error
	if (fread(buffer, 1, signature_size, file) < signature_size) {
		texture.valid = false;
		return;
	}

	// Verify that the signature matches
	if (memcmp(signature, buffer, signature_size) != 0) {
		texture.valid = false;
		return;
	}
}

static ChunkInfo next_chunk_info(FILE* file) {
	ChunkInfo info;
	if (!read_u32(file, info.size)) {
		return info;
	}

	byte header_buffer[ChunkHeader::size];
	// Attempt to read the chunk header.
	if (fread(header_buffer, 1, ChunkHeader::size, file) < ChunkHeader::size) {
		return info; // type is initialized to invalid
	}

	for (auto const& [type, header] : chunk_headers) {
		if (memcmp(header_buffer, header.data, ChunkHeader::size) == 0) {
			info.type = type;
			return info;
		}
	}

	return info;
}

static TextureFormat to_texture_format(byte color_type) {
	switch (color_type) {
	case 0:
		return TextureFormat::greyscale;
	case 2:
		return TextureFormat::rgb;
	case 4:
		return TextureFormat::grayscale_alpha;
	case 6:
		return TextureFormat::rgba;
	default:
		throw "unsupported"; // TODO: Proper error handling
	}
}

// At the end of each chunk there is a CRC section to do validation. We don't do this, se we skip this section in the file.
// See also https://www.w3.org/TR/2003/REC-PNG-20031110/#table51
static void skip_crc(FILE* file) {
	// The CRC is 4 bytes long.
	ignore_bytes(file, 4);
}

static void skip_chunk_contents(FILE* file, ChunkInfo const& info) {
	ignore_bytes(file, info.size);
}

static void process_header_chunk(OpenTexture& tex, FILE* file) {
	read_u32(file, tex.info.width);
	read_u32(file, tex.info.height);

	InternalLoadData* data = reinterpret_cast<InternalLoadData*>(tex.internal_data);
	read_byte(file, data->bit_depth);
	byte color_type = 0;
	read_byte(file, color_type);

	tex.info.format = to_texture_format(color_type);
	ignore_bytes(file, 3);
}

static void process_srgb_chunk(OpenTexture& tex, FILE* file, ChunkInfo const& info) {
	tex.info.color_space = ColorSpace::Srgb;
	// We do not care about the rendering mode byte specified in the srgb chunk data
	skip_chunk_contents(file, info);
}

// extra_data field is meant for processing the IDAT fields. We can pass the resulting array through this parameter.
static void process_chunk(OpenTexture& tex, FILE* file, ChunkInfo const& info, void* extra_data = nullptr) {
	switch (info.type) {
	case ChunkType::IHDR: {
		process_header_chunk(tex, file);
	} break;
	case ChunkType::sRGB: {
		process_srgb_chunk(tex, file, info);
	} break;
	default: {
		// All unprocessed chunks get the contents skipped here
		skip_chunk_contents(file, info);
	} break;
	}

	skip_crc(file);
}

// Processes and discards optional chunks until the first IDAT chunk is encountered
static void process_until_data(OpenTexture& tex, FILE* file) {
	ChunkInfo info = next_chunk_info(file);
	// A PNG file should always start with an IHDR chunk, and since this is the first chunk we load, we can check that
	if (info.type != ChunkType::IHDR) {
		tex.valid = false;
		return;
	}

	while (info.type != ChunkType::IDAT) {
		process_chunk(tex, file, info);
		// Advance to next chunk next chunk
		info = next_chunk_info(file);
	}

	// Now the file pointer is slightly inside the IDAT chunk, so we move it back to the beginning of the chunk.
	// The chunk info takes 8 bytes, so we need to move the file pointer back by 8 bytes
	fseek(file, -(long)ChunkInfo::file_bytes, SEEK_CUR);
}

OpenTexture open_file(std::string_view path) {
	OpenTexture tex;
	FILE* file = fopen(path.data(), "rb");
	tex.file = file;
	check_signature(tex, file);
	if (!tex.valid) { return tex; }

	tex.internal_data = new InternalLoadData;
	
	// Processes IHDR chunk, as well as other optional chunks that preceed the IDAT chunks
	process_until_data(tex, file);

	return tex;
}

uint32_t get_byte_size(OpenTexture const& tex) {
	switch (tex.info.format) {
	case TextureFormat::rgb:
		return tex.info.width * tex.info.height * 3;
	case TextureFormat::rgba:
		return tex.info.width * tex.info.height * 4;
	case TextureFormat::greyscale:
	case TextureFormat::grayscale_alpha:
		throw "not implemented";
	}
}

void load_texture(OpenTexture tex, byte* data) {


	// Cleanup
	delete reinterpret_cast<InternalLoadData*>(tex.internal_data);
	fclose(reinterpret_cast<FILE*>(tex.file));
}



}
}
}
}