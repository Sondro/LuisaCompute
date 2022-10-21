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
    v._size = capacity * ptr->dispatch_indirect_size(capacity) / custom_struct_size;
    return v;
}
Buffer<DrawIndirectArgs> Device::create_draw_indirect_buffer(const MeshFormat &mesh_format, size_t capacity) noexcept {
    Buffer<DrawIndirectArgs> v;
    // Resource
    v._device = _impl;
    auto ptr = _impl.get();
    v._handle = ptr->create_draw_indirect_buffer(mesh_format, false, capacity);
    v._tag = Resource::Tag::BUFFER;
    // Buffer
    v._size = capacity * ptr->draw_indirect_size(mesh_format, false, capacity) / custom_struct_size;
    return v;
}
Buffer<DrawIndexedIndirectArgs> Device::create_indexed_draw_indirect_buffer(const MeshFormat &mesh_format, size_t capacity) noexcept {
    Buffer<DrawIndexedIndirectArgs> v;
    // Resource
    v._device = _impl;
    auto ptr = _impl.get();
    v._handle = ptr->create_draw_indirect_buffer(mesh_format, true, capacity);
    v._tag = Resource::Tag::BUFFER;
    // Buffer
    v._size = capacity * ptr->draw_indirect_size(mesh_format, true, capacity) / custom_struct_size;
    return v;
}
void clear_dispatch_indirect_buffer(BufferVar<DispatchIndirectArgs> const &buffer) {
    detail::FunctionBuilder::current()->call(CallOp::CLEAR_DISPATCH_INDIRECT_BUFFER, {buffer.expression()});
}
void emplace_indirect_dispatch_kernel1d(
    BufferVar<DispatchIndirectArgs> const &buffer,
    Expr<uint> block_size,
    Expr<uint> dispatch_size,
    Expr<uint> kernel_id) {
    detail::FunctionBuilder::current()->call(CallOp::EMPLACE_DISPATCH_INDIRECT_KERNEL1D, {buffer.expression(), block_size.expression(), dispatch_size.expression(), kernel_id.expression()});
}
void emplace_indirect_dispatch_kernel2d(
    BufferVar<DispatchIndirectArgs> const &buffer,
    Expr<uint2> block_size,
    Expr<uint2> dispatch_size,
    Expr<uint> kernel_id) {
    detail::FunctionBuilder::current()->call(CallOp::EMPLACE_DISPATCH_INDIRECT_KERNEL2D, {buffer.expression(), block_size.expression(), dispatch_size.expression(), kernel_id.expression()});
}
void emplace_indirect_dispatch_kernel3d(
    BufferVar<DispatchIndirectArgs> const &buffer,
    Expr<uint3> block_size,
    Expr<uint3> dispatch_size,
    Expr<uint> kernel_id) {
    detail::FunctionBuilder::current()->call(CallOp::EMPLACE_DISPATCH_INDIRECT_KERNEL3D, {buffer.expression(), block_size.expression(), dispatch_size.expression(), kernel_id.expression()});
}
}// namespace luisa::compute