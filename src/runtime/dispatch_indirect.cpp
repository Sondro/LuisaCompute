#include <dsl/dispatch_indirect.h>
#include <runtime/device.h>
#include <runtime/buffer.h>
namespace luisa::compute {
Buffer<DispatchIndirectArgs> Device::create_dispatch_indirect_buffer(size_t capacity) noexcept {
    Buffer<DispatchIndirectArgs> v;
    // Resource
    v._device = _impl;
    auto ptr = _impl.get();
    v._handle = ptr->create_dispatch_indirect_buffer(capacity);
    v._tag = Resource::Tag::BUFFER;
    // Buffer
    v._size = capacity * ptr->dispatch_indirect_size(capacity);
    return v;
}
}// namespace luisa::compute