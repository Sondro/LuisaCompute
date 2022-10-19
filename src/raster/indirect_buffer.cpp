/*
#include <raster/indirect_buffer.h>
#include <runtime/device.h>
namespace luisa::compute {
IndirectBuffer::IndirectBuffer(
    DeviceInterface *device,
    const MeshFormat &mesh_format,
    size_t size) noexcept
    : Resource(device, Resource::Tag::INDIRECT_BUFFER, device->create_indirect_buffer(mesh_format, size)),
      _size(size) {
}
void IndirectBuffer::set(size_t idx, RasterMesh &&raster_mesh) noexcept {
    _modifications.emplace_back(idx, std::move(raster_mesh));
}
luisa::vector<std::pair<size_t, RasterMesh>> IndirectBuffer::steal_modifications() {
    auto ext = vstd::scope_exit([&] { vstd::reset(_modifications); });
    return std::move(_modifications);
}

}// namespace luisa::compute
*/