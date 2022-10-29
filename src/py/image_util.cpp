#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <runtime/image.h>
#include <py/py_stream.h>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

namespace py = pybind11;
using namespace luisa::compute;
const auto pyref = py::return_value_policy::reference;// object lifetime is managed on C++ side
void export_img(py::module &m) {
    m.def("load_image", [](std::string &&path) {
        int32_t x, y, channel;
        auto ptr = stbi_loadf(path.c_str(), &x, &y, &channel, 0);
        py::capsule free_when_done(ptr, vengine_free);
        return py::array_t<float>(
            {x, y, channel},
            {y * channel * sizeof(float), channel * sizeof(float), sizeof(float)},
            ptr,
            std::move(free_when_done));
    });
}