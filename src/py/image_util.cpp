#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include <runtime/image.h>
#include <py/py_stream.h>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

namespace py = pybind11;
using namespace luisa::compute;
const auto pyref = py::return_value_policy::reference;// object lifetime is managed on C++ side
struct FloatImg {
    int32_t x, y, channel;
    luisa::shared_ptr<float> ptr;
    size_t size() const noexcept {
        return static_cast<size_t>(x) * static_cast<size_t>(y) * static_cast<size_t>(channel);
    }
};
FloatImg read_hdr_tex(char const *name, int32_t desireChannel) {
    FloatImg tex;
    tex.ptr = luisa::shared_ptr<float>(stbi_loadf(name, &tex.x, &tex.y, &tex.channel, desireChannel));
    return tex;
}
void export_img(py::module &m) {
    py::class_<FloatImg>(m, "FloatImg")
        .def(py::init([](std::string &&name, int32_t desireChannel) {
            return read_hdr_tex(name.c_str(), desireChannel);
        }))
        .def("width", [](FloatImg &img) { return img.x; })
        .def("height", [](FloatImg &img) { return img.y; })
        .def("channel", [](FloatImg &img) { return img.channel; })
        .def("size", [](FloatImg &img) { return img.size(); })
        .def("buffer_upload", [](FloatImg &img, PyStream& stream, uint64_t buffer_handle) {
            stream.add(BufferUploadCommand::create(buffer_handle, 0, img.size() * sizeof(float), img.ptr.get()));
            auto newPtr = img.ptr;
            stream.add_upload(std::move(newPtr));
        }, pyref);
}