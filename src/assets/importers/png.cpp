#include <andromeda/assets/importers/png.hpp>

#include <stdio.h>
#include <unordered_map>

#include <zlib.h>

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
	// These chunks are optional
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

static bool read_bytes(FILE* file, byte* values, uint32_t count) {
	if (fread(values, 1, count, file) < count) {
		return false;
	}
	return true;
}

// Ignores count bytes from the file
static bool ignore_bytes(FILE* file, size_t count) {
	return fseek(file, count, SEEK_CUR) == 0;
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

static uint32_t bytes_per_pixel(TextureFormat fmt) {
	switch (fmt) {
	case TextureFormat::rgb: return 3;
	case TextureFormat::rgba: return 4;
	case TextureFormat::greyscale:
	case TextureFormat::grayscale_alpha:
		throw "unsupported";
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
	tex.info.byte_width = tex.info.width * bytes_per_pixel(tex.info.format);
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

uint32_t get_required_size(OpenTexture const& tex) {
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

class DecompressionStream {
public:
	DecompressionStream(FILE* file, byte* dst_buffer, uint32_t dst_buffer_size) 
		: file(file), dst_buffer_size(dst_buffer_size), dst_buffer(dst_buffer) {
		working_memory_size = find_largest_chunk(file);
		working_memory = new byte[working_memory_size];
	}

	~DecompressionStream() {
		delete working_memory;
	}

	bool decompress() {
		z_stream decompress_job{};
		decompress_job.next_out = dst_buffer;
		decompress_job.avail_out = dst_buffer_size;
		if (inflateInit(&decompress_job) != Z_OK) {
			inflateEnd(&decompress_job);
			return false;
		}

		ChunkInfo chunk = next_chunk_info(file);
		while (chunk.type == ChunkType::IDAT) {

			if (chunk.size > working_memory_size) {
				// Not enough memory to decompress chunk
				inflateEnd(&decompress_job);
				return false;
			}

			// Read compressed data into memroy
			if (!read_bytes(file, working_memory, chunk.size)) {
				// Data could not be read
				inflateEnd(&decompress_job);
				return false;
			}

			// Decompress
			decompress_job.next_in = working_memory;
			decompress_job.avail_in = chunk.size;
			int err_code = inflate(&decompress_job, 0);
			if (err_code == Z_DATA_ERROR) {
				inflateEnd(&decompress_job);
				return false;
			}
			// Note that inflate() automatically updates next_out and avail_out, so we don't need any extra logic to update these

			skip_crc(file);
			chunk = next_chunk_info(file);
		}

		inflateEnd(&decompress_job);
		// Seek slightly back to avoid the file pointer already being inside the next chunk
		fseek(file, -(long)ChunkInfo::file_bytes, SEEK_CUR);
		
		return true;
	}

private:
	FILE* file;
	uint32_t dst_buffer_size;
	byte* dst_buffer;
	uint32_t working_memory_size = 0;
	byte* working_memory = nullptr;

	uint32_t find_largest_chunk(FILE* file) {
		auto first_idat_pos = ftell(file);

		uint32_t size = 0;
		ChunkInfo info = next_chunk_info(file);
		while (info.type == ChunkType::IDAT) {
			if (info.size > size) { size = info.size; }
			skip_chunk_contents(file, info);
			skip_crc(file);
			info = next_chunk_info(file);
		}

		// Seek back to beginning of the data chunks
		fseek(file, first_idat_pos, SEEK_SET);

		return size;
	}
};

enum class FilterType {
	None = 0,
	Sub = 1,
	Up = 2,
	Average = 3,
	Paeth = 4
};

static byte& access_filtered_2d(OpenTexture const& tex, byte* data, uint32_t x, uint32_t y) {
	// In the filtered array, the pixel data is offset by one because of the leading filter type byte
	return data[y * tex.info.byte_width + x + 1];
}

static byte& access_defiltered_2d(OpenTexture const& tex, byte* data, uint32_t x, uint32_t y) {
	return data[y * tex.info.byte_width + x];
}

// Returns a pointer to the first pixel in the row
static byte* get_filtered_row_pointer(OpenTexture const& tex, byte* data, uint32_t row) {
	return data + (size_t)row * ((size_t)tex.info.byte_width + 1) + 1;
}

// Returns a pointer to the first pixel in the row
static byte* get_defiltered_row_pointer(OpenTexture const& tex, byte* data, uint32_t row) {
	return data + (size_t)row * tex.info.byte_width;
}

static FilterType get_filter_type(OpenTexture const& tex, byte* data, uint32_t row) {
	// The first value in a filtered row is the filter type
	return static_cast<FilterType>(data[row * (tex.info.byte_width + 1)]);
}

static byte filter_add(byte lhs, byte rhs) {
	uint16_t result = (uint16_t)(lhs)+(uint16_t)(rhs);
	result %= 256;
	return static_cast<byte>(result);
}

// Defilter row using the 'up' method. src must point to the first byte in the filtered row, and dst must point to the first byte for this
// row in the defiltered buffer
static void defilter_up(OpenTexture const& tex, byte* src, byte* dst, uint32_t row) {
	// Get previous row
	byte const* up = dst - tex.info.byte_width;
	for (byte const* p = src; p != src + tex.info.byte_width; ++p) {
		// Add values together
		*dst = filter_add(*p, *up);
		// Increment destination and up pointer to the next byte
		++up;
		++dst;
	}
}

// Defilter row using the 'sub' method. src must point to the first byte in the filtered row, and dst must point to the first byte for this
// row in the defiltered buffer
static void defilter_sub(OpenTexture const& tex, byte* src, byte* dst, uint32_t row) {
	// To revert the sub filter, we take the previous byte and add it to this byte. For the first byte, the previous byte is defined to be 0.
	byte a = 0;
	for (byte const* p = src; p != src + tex.info.byte_width; ++p) {
		*dst = filter_add(*p, a);
		a = *dst;
		++dst;
	}
}
// Defilter row using the 'average' method. src must point to the first byte in the filtered row, and dst must point to the first byte for this
// row in the defiltered buffer
static void defilter_average(OpenTexture const& tex, byte* src, byte* dst, uint32_t row) {

}

static bool defilter_first_row(OpenTexture const& tex, byte* src, byte* dst) {
	// Start by defiltering the first row, as this top row is slightly different.
	// Defiltering requires access to the defiltered values of the previous row, and of the previous pixel.
	// Since the top row has no rows above it, it is treated as if there was a row with zeros above it.
	FilterType type = get_filter_type(tex, src, 0);
	byte* src_ptr = get_filtered_row_pointer(tex, src, 0);
	byte* dst_ptr = get_defiltered_row_pointer(tex, dst, 0);
	switch (type) {
	case FilterType::None: {
		// No filtering needs to be applied, so we just need to get rid of the leading filter byte
		memcpy(dst_ptr, src_ptr, tex.info.byte_width);
	} break;
	case FilterType::Up: {
		// For the Up filter, we have to defilter by adding the corresponding values from the previous row. 
		// Since this is the first row, we pretend there is a row of zeros above it, and this operation is translated into a single memcpy
		memcpy(dst_ptr, src_ptr, tex.info.byte_width);
	} break;
	case FilterType::Sub: {
		defilter_sub(tex, src_ptr, dst_ptr, 0);
	} break;
	case FilterType::Average: {
		int j = 0;
	} break;
	case FilterType::Paeth: {
		int i = 0;
	} break;
	default:
		return false;
	}

	return true;
}

static bool defilter_data(OpenTexture const& tex, byte* src, byte* dst) {
	// Filtering documentation can be found at https://www.w3.org/TR/2003/REC-PNG-20031110/#9Filters
	if (!defilter_first_row(tex, src, dst)) { return false; }
	// Defilter other rows
	for (uint32_t row = 1; row < tex.info.height; ++row) {
		FilterType type = get_filter_type(tex, src, row);
		byte* src_ptr = get_filtered_row_pointer(tex, src, row);
		byte* dst_ptr = get_defiltered_row_pointer(tex, dst, row);
		switch (type) {
		case FilterType::None: {
			// No filtering needs to be applied, so we just need to get rid of the leading filter byte
			memcpy(dst_ptr, src_ptr, tex.info.byte_width);
		} break;
		case FilterType::Up: {
			// For the Up filter, we have to defilter by adding the corresponding values from the previous row. 
			defilter_up(tex, src_ptr, dst_ptr, row);
		} break;
		case FilterType::Sub: {
			defilter_sub(tex, src_ptr, dst_ptr, row);
		} break;
		case FilterType::Average: {
			int j = 0;
		} break;
		case FilterType::Paeth: {
			int i = 0;
		} break;
		default:
			return false;
		}
	}

	return true;
}

bool load_texture(OpenTexture tex, byte* data) {
	FILE* file = reinterpret_cast<FILE*>(tex.file);
	InternalLoadData* internal_data = reinterpret_cast<InternalLoadData*>(tex.internal_data);

	uint32_t filtered_size = get_required_size(tex) + tex.info.height;
	byte* filtered = new byte[filtered_size];
	DecompressionStream stream(file, filtered, filtered_size);
	if (!stream.decompress()) {
		delete[] filtered;
		delete internal_data;
		fclose(file);
		return false;
	}

	// Now that the data is decompressed, we have to defilter it. After this operation, exactly height bytes will be unused
	// in the final image data, since defiltering removes the leading filter method bytes.
	defilter_data(tex, filtered, data);

	delete[] filtered;
	delete internal_data;
	fclose(file);
	return true;
}



}
}
}
}