#include <raster/raster_kernel.h>
#include <raster/raster_shader.h>
#include <raster/depth_format.h>
#include <raster/depth_buffer.h>
#include <raster/raster_scene.h>
#include <rtx/accel.h>
#include <runtime/bindless_array.h>
namespace luisa::compute {
// see definition in rtx/accel.cpp
RasterShaderInvoke &RasterShaderInvoke::operator<<(const Accel &accel) noexcept {
    _command->encode_accel(accel.handle());
    return *this;
}

// see definition in runtime/bindless_array.cpp
RasterShaderInvoke &RasterShaderInvoke::operator<<(const BindlessArray &array) noexcept {
    _command->encode_bindless_array(array.handle());
    return *this;
}
#ifndef NDEBUG
void RasterShaderInvoke::check_dst(luisa::span<PixelFormat const> rt_formats, DepthBuffer const *depth) {
    if (_rtv_format.size() != rt_formats.size()) {
        LUISA_ERROR("Render target count mismatch!");
    }
    for (size_t i = 0; i < rt_formats.size(); ++i) {
        if (_rtv_format[i] != rt_formats[i]) {
            LUISA_ERROR("Render target {} format mismatch", char(i + 48));
        }
    }
    if ((depth && depth->format() != _dsv_format) || (!depth && _dsv_format != DepthFormat::None)) {
        LUISA_ERROR("Depth-buffer's format mismatch");
    }
}
void RasterShaderInvoke::check_scene(RasterScene *scene) {
    for (auto &&mesh : scene->meshes) {
        auto vb = mesh.vertex_buffers();
        if (vb.size() != _mesh_format->vertex_stream_count()) {
            LUISA_ERROR("Vertex buffer count mismatch!");
        }
        for (size_t i = 0; i < vb.size(); ++i) {
            auto stream = _mesh_format->attributes(i);
            size_t target_stride = 0;
            for(auto& s : stream){
                target_stride += VertexElementFormatStride(s.format);
            }
            if(target_stride != vb[i].stride()){
                LUISA_ERROR("Vertex buffer {} stride mismatch!", char(i + 48));
            }
        }
    }
}
#endif

}// namespace luisa::compute