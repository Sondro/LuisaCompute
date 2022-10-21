#include <dsl/dispatch_indirect.h>
#include <runtime/device.h>
#include <runtime/buffer.h>
namespace luisa::compute {
template<size_t i, typename T>
Buffer<T> Device::create_dispatch_buffer(size_t capacity) noexcept {
    Buffer<T> v;
    // Resource
    v._device = _impl;
    auto ptr = _impl.get();
    v._handle = ptr->create_dispatch_buffer(i, capacity);
    v._tag = Resource::Tag::BUFFER;
    // Buffer
    v._size = capacity * ptr->dispatch_buffer_size(i, capacity) / custom_struct_size;
    return v;
};

Buffer<DispatchArgs1D> Device::create_1d_dispatch_buffer(size_t capacity) noexcept {
    return create_dispatch_buffer<1, DispatchArgs1D>(capacity);
}
Buffer<DispatchArgs2D> Device::create_2d_dispatch_buffer(size_t capacity) noexcept {
    return create_dispatch_buffer<2, DispatchArgs2D>(capacity);
}
Buffer<DispatchArgs3D> Device::create_3d_dispatch_buffer(size_t capacity) noexcept {
    return create_dispatch_buffer<3, DispatchArgs3D>(capacity);
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
void clear_dispatch_buffer(Expr<Buffer<DispatchArgs1D>> buffer) {
    detail::FunctionBuilder::current()->call(CallOp::CLEAR_DISPATCH_INDIRECT_BUFFER, {buffer.expression()});
}
void clear_dispatch_buffer(Expr<Buffer<DispatchArgs2D>> buffer) {
    detail::FunctionBuilder::current()->call(CallOp::CLEAR_DISPATCH_INDIRECT_BUFFER, {buffer.expression()});
}
void clear_dispatch_buffer(Expr<Buffer<DispatchArgs3D>> buffer) {
    detail::FunctionBuilder::current()->call(CallOp::CLEAR_DISPATCH_INDIRECT_BUFFER, {buffer.expression()});
}
void emplace_dispatch_kernel(
    Expr<Buffer<DispatchArgs1D>> buffer,
    Expr<uint> block_size,
    Expr<uint> dispatch_size,
    Expr<uint> kernel_id) {
    detail::FunctionBuilder::current()->call(CallOp::EMPLACE_DISPATCH_INDIRECT_KERNEL, {buffer.expression(), block_size.expression(), dispatch_size.expression(), kernel_id.expression()});
}
void emplace_dispatch_kernel(
    Expr<Buffer<DispatchArgs2D>> buffer,
    Expr<uint2> block_size,
    Expr<uint2> dispatch_size,
    Expr<uint> kernel_id) {
    detail::FunctionBuilder::current()->call(CallOp::EMPLACE_DISPATCH_INDIRECT_KERNEL, {buffer.expression(), block_size.expression(), dispatch_size.expression(), kernel_id.expression()});
}
void emplace_dispatch_kernel(
    Expr<Buffer<DispatchArgs3D>> buffer,
    Expr<uint3> block_size,
    Expr<uint3> dispatch_size,
    Expr<uint> kernel_id) {
    detail::FunctionBuilder::current()->call(CallOp::EMPLACE_DISPATCH_INDIRECT_KERNEL, {buffer.expression(), block_size.expression(), dispatch_size.expression(), kernel_id.expression()});
}
}// namespace luisa::compute