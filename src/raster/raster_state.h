#pragma once
#include <raster/vertex_attribute.h>
#include <core/stl/optional.h>
namespace luisa::compute {
enum class DepthFormat : uint8_t {
    D16,
    D24S8,
    D32,
    D32S8A24
};
enum class Comparison : uint8_t {
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};
class MeshFormat {
private:
    luisa::vector<luisa::vector<VertexAttribute>> _streams;

public:
    size_t vertex_attribute_count() const {
        size_t count = 0;
        for (auto &&i : _streams) {
            count += i.size();
        }
        return count;
    }
    size_t vertex_stream_count() const { return _streams.size(); }
    void emplace_vertex_stream(luisa::span<VertexAttribute const> attributes) {
        _streams.emplace_back(attributes.begin(), attributes.end());
    }
    luisa::span<VertexAttribute const> attributes(size_t stream_index) const { return _streams[stream_index]; }
    decltype(auto) begin() const { return _streams.begin(); }
    decltype(auto) end() const { return _streams.end(); }
};
enum class FillMode : uint8_t {
    WireFrame,
    Solid
};
enum class CullMode : uint8_t {
    None,
    Front,
    Back
};
enum class TopologyType : uint8_t {
    Point,
    Line,
    Triangle
};
enum class BlendOp : uint8_t {
    Zero,
    One,
    SrcColor,
    DstColor,
    SrcAlpha,
    DstAlpha,
    OneMinusSrcColor,
    OneMinusDstColor,
    OneMinusSrcAlpha,
    OneMinusDstAlpha
};
enum class StencilOp : uint8_t {
    Keep,
    Zero,
    Replace
};

struct BlendState {
    BlendOp src_op{BlendOp::Zero};
    BlendOp dst_op{BlendOp::One};
};
struct DepthState {
    Comparison comparison{};
    bool write{};
};
struct StencilFaceOp {
    StencilOp stencil_fail_op{};
    StencilOp depth_fail_op{};
    StencilOp pass_op{};
    Comparison comparison{};
};
struct StencilState {
    StencilFaceOp front_face_op{};
    StencilFaceOp back_face_op{};
    uint8_t read_mask{};
    uint8_t write_mask{};
};
struct RasterState {
    FillMode fill_mode{FillMode::Solid};
    CullMode cull_mode{CullMode::Back};
    luisa::optional<BlendState> blend_state{};
    luisa::optional<DepthState> depth_state{};
    luisa::optional<StencilState> stencil_state{};
    TopologyType topology{TopologyType::Triangle};
    bool front_counter_clockwise{false};
    bool depth_clip{true};
    bool conservative{false};
};
// Make raster state hash-able
static_assert(alignof(RasterState) == 1);
}// namespace luisa::compute