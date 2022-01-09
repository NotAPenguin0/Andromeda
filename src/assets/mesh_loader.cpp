#include <andromeda/assets/loaders.hpp>

#include <andromeda/graphics/context.hpp>

#include <assetlib/asset_file.hpp>
#include <assetlib/mesh.hpp>


#include <string>

namespace andromeda {
namespace impl {

void load_mesh(gfx::Context& ctx, Handle<gfx::Mesh> handle, std::string_view path, uint32_t thread) {
    using namespace std::literals::string_literals;

    assetlib::AssetFile file{};
    plib::binary_input_stream in_stream = plib::binary_input_stream::from_file(path.data());
    bool success = assetlib::load_binary_file(in_stream, file);
    if (!success) {
        LOG_FORMAT(LogLevel::Error, "Failed to open mesh file {}", path);
        return;
    }

    assetlib::MeshInfo info = assetlib::read_mesh_info(file);

    if (info.format != assetlib::VertexFormat::PNTV32) {
        LOG_WRITE(LogLevel::Error, "Tried to load mesh with unsupported vertex format");
        return;
    }

    if (info.index_bits != 32) {
        LOG_FORMAT(LogLevel::Error, "Tried to load mesh with invalid index type. "
                                    "Only 32-bit indices are supported, this mesh has {}-bit indices", info.index_bits);
        return;
    }

    gfx::Mesh mesh{};
    mesh.num_vertices = info.vertex_count;
    mesh.num_indices = info.index_count;

    mesh.vertices = ctx.create_buffer(ph::BufferType::VertexBuffer, info.vertex_count * sizeof(assetlib::PNTV32Vertex));
    mesh.indices = ctx.create_buffer(ph::BufferType::IndexBuffer, info.index_count * sizeof(uint32_t));

    ctx.name_object(mesh.vertices.handle, path.data() + " - Vertex Buffer"s);
    ctx.name_object(mesh.indices.handle, path.data() + " - Index Buffer"s);

    // Create staging buffers
    ph::RawBuffer vtx_staging = ctx.create_buffer(ph::BufferType::TransferBuffer, mesh.vertices.size);
    ph::RawBuffer idx_staging = ctx.create_buffer(ph::BufferType::TransferBuffer, mesh.indices.size);

    // Unpack directly into staging memory
    std::byte* vtx = ctx.map_memory(vtx_staging);
    std::byte* idx = ctx.map_memory(idx_staging);
    assetlib::unpack_mesh(info, file, vtx, idx);

    // Copy from staging buffers to GPU buffers.
    ph::Queue& transfer = *ctx.get_queue(ph::QueueType::Transfer);

    ph::CommandBuffer cmd_buf = transfer.begin_single_time(thread);
    cmd_buf.copy_buffer(vtx_staging, mesh.vertices);
    cmd_buf.copy_buffer(idx_staging, mesh.indices);

    VkFence fence = ctx.create_fence();
    transfer.end_single_time(cmd_buf, fence);
    ctx.wait_for_fence(fence);

    ctx.destroy_buffer(vtx_staging);
    ctx.destroy_buffer(idx_staging);
    ctx.destroy_fence(fence);
    transfer.free_single_time(cmd_buf, thread);

    assets::impl::make_ready(handle, std::move(mesh));

    LOG_FORMAT(LogLevel::Info, "Loaded mesh {}", path);
    float size_mb = (float) (info.vertex_count * sizeof(assetlib::PNTV32Vertex) + info.index_count * sizeof(uint32_t)) / (1024.0f * 1024.0f);
    if (size_mb < 1.0f) {
        float size_kb = size_mb * 1024.0f;
        LOG_FORMAT(LogLevel::Performance, "Mesh {} loaded with {} vertices and {} triangles ({:.1f} KiB)",
                   path, info.vertex_count, info.index_count / 3,
                   size_kb);
    } else {
        LOG_FORMAT(LogLevel::Performance, "Mesh {} loaded with {} vertices and {} triangles ({:.1f} MiB)",
                   path, info.vertex_count, info.index_count / 3,
                   size_mb);
    }
}

} // namespace impl
} // namespace andromeda