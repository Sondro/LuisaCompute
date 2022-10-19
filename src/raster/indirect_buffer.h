#pragma once
#include <runtime/resource.h>
#include <raster/raster_state.h>
#include <raster/raster_scene.h>
#include <dsl/var.h>
namespace luisa::compute {
class IndirectBuffer;

class LC_RUNTIME_API IndirectBuffer : public Resource {
    luisa::vector<std::pair<size_t, RasterMesh>> _modifications;
    size_t _size;
    luisa::vector<std::pair<size_t, RasterMesh>> steal_modifications();

public:
    IndirectBuffer(
        DeviceInterface *device,
        const MeshFormat &mesh_format,
        size_t size) noexcept;
    void set(size_t idx, RasterMesh &&raster_mesh) noexcept;
    // DSL
    [[nodiscard]] void copy_from(Expr<uint> src, Expr<IndirectBuffer> buffer, Expr<uint> dst) const noexcept;
};
template<>
struct LC_RUNTIME_API Var<IndirectBuffer> {
};
}// namespace luisa::compute