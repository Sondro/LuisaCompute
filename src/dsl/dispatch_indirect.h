#pragma once
#include <dsl/expr.h>
#include <dsl/var.h>
#include <dsl/struct.h>
#include <dsl/custom_struct.h>
LUISA_CUSTOM_STRUCT(DispatchArgs1D);
LUISA_CUSTOM_STRUCT(DispatchArgs2D);
LUISA_CUSTOM_STRUCT(DispatchArgs3D);
namespace luisa::compute {
LC_DSL_API void clear_dispatch_buffer(BufferVar<DispatchArgs1D> const &buffer);
LC_DSL_API void clear_dispatch_buffer(BufferVar<DispatchArgs2D> const &buffer);
LC_DSL_API void clear_dispatch_buffer(BufferVar<DispatchArgs3D> const &buffer);
LC_DSL_API void emplace_dispatch_kernel(
    BufferVar<DispatchArgs1D> const &buffer,
    Expr<uint> block_size,
    Expr<uint> dispatch_size,
    Expr<uint> kernel_id);
LC_DSL_API void emplace_dispatch_kernel(
    BufferVar<DispatchArgs2D> const &buffer,
    Expr<uint2> block_size,
    Expr<uint2> dispatch_size,
    Expr<uint> kernel_id);
LC_DSL_API void emplace_dispatch_kernel(
    BufferVar<DispatchArgs3D> const &buffer,
    Expr<uint3> block_size,
    Expr<uint3> dispatch_size,
    Expr<uint> kernel_id);
template<typename... Args>
inline void emplace_dispatch_kernel(
    BufferVar<DispatchArgs1D> const &buffer, Kernel1D<Args...> const &kernel, Expr<uint> dispatch_size, Expr<uint> kernel_id) {
    emplace_dispatch_kernel(
        buffer,
        kernel.function()->block_size().x,
        dispatch_size,
        kernel_id);
}
template<typename... Args>
inline void emplace_dispatch_kernel(
    BufferVar<DispatchArgs2D> const &buffer, Kernel2D<Args...> const &kernel, Expr<uint> dispatch_size, Expr<uint> kernel_id) {
    emplace_dispatch_kernel(
        buffer,
        kernel.function()->block_size().xy(),
        dispatch_size,
        kernel_id);
}
template<typename... Args>
inline void emplace_dispatch_kernel(
    BufferVar<DispatchArgs3D> const &buffer, Kernel3D<Args...> const &kernel, Expr<uint> dispatch_size, Expr<uint> kernel_id) {
    emplace_dispatch_kernel(
        buffer,
        kernel.function()->block_size(),
        dispatch_size,
        kernel_id);
}
}// namespace luisa::compute