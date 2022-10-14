
#include <iostream>

#include <runtime/context.h>
#include <runtime/device.h>
#include <runtime/stream.h>
#include <runtime/event.h>
#include <vstl/StringUtility.h>
#include <vstl/BinaryReader.h>
#include <raster/raster_shader.h>
#include <raster/dsl.h>

using namespace luisa;
using namespace luisa::compute;
struct VV{
    size_t a;
};
int main(int argc, char *argv[]) {
    log_level_info();

    Context context{argv[0]};
    auto device = context.create_device("dx");
    Callable vert = []() noexcept {
        auto vert = get_vertex_data();
        return make_float4(vert.position, 1.0f);
    };
    Callable pixel = [](Float4 v2p) noexcept {
        return make_float3(0.5);
    };
    RasterKernel<decltype(vert), decltype(pixel)> kernel(vert, pixel);
    auto vertAttribs = {
        VertexAttribute{
            .type = VertexAttributeType::Position,
            .format = VertexElementFormat::XYZ32Float},
        VertexAttribute{
            .type = VertexAttributeType::Normal,
            .format = VertexElementFormat::XYZ32Float}};
    MeshFormat meshFormat;
    meshFormat.emplace_vertex_stream(vertAttribs);
    RasterState rasterState;
    auto dstFormat = PixelFormat::RGBA8UNorm;
    auto shader = device.compile(kernel, meshFormat, rasterState, {&dstFormat, size_t(1)}, DepthFormat::D32);
    
    // auto &&ctx = device.context();
    // auto &&module = ctx.loaded_modules()[ctx.index()];
    // auto consoleTest = module.function<void()>("ConsoleTest");
    // consoleTest();
}