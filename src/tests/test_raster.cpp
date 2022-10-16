
#include <iostream>

#include <runtime/context.h>
#include <raster/depth_buffer.h>
#include <runtime/device.h>
#include <runtime/stream.h>
#include <runtime/event.h>
#include <vstl/StringUtility.h>
#include <vstl/BinaryReader.h>
#include <raster/raster_shader.h>
#include <raster/dsl.h>
#include <raster/raster_state.h>
#include <raster/raster_scene.h>
#include <dsl/func.h>
#include <stb/stb_image_write.h>
#include <dsl/syntax.h>
#include <dsl/sugar.h>

using namespace luisa;
using namespace luisa::compute;
struct PackedFloat3 {
    float v[3];
    float n[4];
};
LUISA_STRUCT(PackedFloat3, v, n){};
struct v2p {
    float4 pos;
    float4 color;
};
LUISA_STRUCT(v2p, pos, color){};
int main(int argc, char *argv[]) {
    log_level_info();
    constexpr uint width = 1024;
    constexpr uint height = 1024;

    Context context{argv[0]};
    auto device = context.create_device("dx");
    Callable vert = []() noexcept {
        auto vert = get_vertex_data();
        Var<v2p> o;
        vert.position *= make_float3(0.5);
        switch_(vert.instance_id)
            .case_(0, [&] {
                vert.position -= make_float3(0.5, 0, 0.1);
            })
            .case_(1, [&] {
                vert.position += make_float3(0.5, 0, 0.1);
            });
        switch_(object_id())
            .case_(0, [&] {
                vert.position -= make_float3(0, 0.5, 0.2);
            })
            .case_(1, [&] {
                vert.position += make_float3(0, 0.5, 0.2);
            });
        o.pos = make_float4(vert.position, 1.0f);
        o.color = vert.color;
        return o;
    };
    Callable pixel = [](Var<v2p> v2p) noexcept {
        switch_(object_id())
            .case_(0, [&] {
                $return(v2p.color);
            })
            .case_(1, [&] {
                $return(make_float4(make_float3(1) - v2p.color.xyz(), 1.0f));
            });
        return make_float4(0, 0, 0, 1);
    };
    Callable linear_to_srgb = [](Var<float3> x) noexcept {
        return select(1.055f * pow(x, 1.0f / 2.4f) - 0.055f,
                      12.92f * x,
                      x <= 0.00031308f);
    };

    Kernel2D clearRT = [&](ImageVar<float> img) {
        img.write(dispatch_id().xy(), make_float4(0.3, 0.5, 0.7, 1));
    };
    Kernel2D printRT = [&](ImageVar<float> img, BufferVar<uint> ldrBuffer) {
        auto i = dispatch_y() * dispatch_size_x() + dispatch_x();
        auto hdr = img.read(dispatch_id().xy()).xyz();
        auto ldr = make_uint3(round(saturate(linear_to_srgb(hdr)) * 255.0f));
        ldrBuffer.write(i, ldr.x | (ldr.y << 8u) | (ldr.z << 16u) | (255u << 24u));
    };
    RasterKernel<decltype(vert), decltype(pixel)> kernel(vert, pixel);
    auto vertAttribs = {
        VertexAttribute{
            .type = VertexAttributeType::Position,
            .format = VertexElementFormat::XYZ32Float},
        VertexAttribute{
            .type = VertexAttributeType::Color,
            .format = VertexElementFormat::XYZW32Float}};
    MeshFormat meshFormat;
    meshFormat.emplace_vertex_stream(vertAttribs);
    RasterState rasterState{
        .cull_mode = CullMode::None,
        .depth_state = {
            .enableDepth = true,
            .write = true,
            .comparison = Comparison::Less}};
    auto depth = device.create_depth_buffer(DepthFormat::D32, uint2(width, height));
    auto tex = device.create_image<float>(PixelStorage::BYTE4, uint2(width, height));
    auto dstFormat = tex.format();
    auto shader = device.compile(kernel, meshFormat, rasterState, {&dstFormat, size_t(1)}, DepthFormat::D32);
    auto printShader = device.compile(printRT);
    auto clearShader = device.compile(clearRT);

    auto stream = device.create_stream(StreamTag::GRAPHICS);
    auto resultBuffer = device.create_buffer<uint>(width * height);
    auto vb = device.create_buffer<PackedFloat3>(3);
    auto ib = device.create_buffer<uint>(3);
    VertexBufferView vbv(vb);
    RasterScene scene;
    // Emplace two triangles
    scene.meshes.emplace_back(luisa::span<VertexBufferView const>{&vbv, 1ull}, ib, 2);
    scene.meshes.emplace_back(luisa::span<VertexBufferView const>{&vbv, 1ull}, ib, 2);
    float vertPoses[sizeof(PackedFloat3) * 3] = {
        -1, -1, 0.1,
        1, 0, 0, 1,
        0, 1, 1,
        0, 1, 0, 1,
        1, -1, 0.2,
        0, 0, 1, 1};
    uint indices[3] = {0, 1, 2};
    Viewport viewport{
        .start = {0, 0},
        .size = {1, 1}};
    vstd::vector<uint> pixels(width * height);
    stream
        << vb.copy_from(vertPoses) << ib.copy_from(indices)
        << clearShader(tex).dispatch(width, height) << depth.clear(0.5)
        << shader().draw(&scene, viewport, &depth, tex)
        << printShader(tex, resultBuffer).dispatch(width, height)
        << resultBuffer.copy_to(pixels.data())
        << synchronize();
    stbi_write_png("test_raster.png", width, height, 4, pixels.data(), 0);

    // auto &&ctx = device.context();
    // auto &&module = ctx.loaded_modules()[ctx.index()];
    // auto consoleTest = module.function<void()>("ConsoleTest");
    // consoleTest();
}