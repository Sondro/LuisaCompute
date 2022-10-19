#include <lc_api/lc_device.h>
VSTL_EXPORT_C luisa::compute::DeviceInterface *create(Context const &c, std::string_view) {
    return new toolhub::vk::LCDevice(c);
}
VSTL_EXPORT_C void destroy(luisa::compute::DeviceInterface *device) {
    delete static_cast<toolhub::vk::LCDevice *>(device);
}