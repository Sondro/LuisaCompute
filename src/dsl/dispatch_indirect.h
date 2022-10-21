#pragma once
#include <dsl/expr.h>
#include <dsl/var.h>
#include <dsl/struct.h>
#include <dsl/custom_struct.h>
LUISA_CUSTOM_STRUCT(DispatchIndirectArgs);
namespace luisa::compute {
LC_DSL_API void clear_dispatch_indirect_buffer(BufferVar<DispatchIndirectArgs> const &buffer);
LC_DSL_API void emplace_indirect_dispatch_kernel1d(
    BufferVar<DispatchIndirectArgs> const &buffer,
    Expr<uint> block_size,
    Expr<uint> dispatch_size,
    Expr<uint> kernel_id);
LC_DSL_API void emplace_indirect_dispatch_kernel2d(
    BufferVar<DispatchIndirectArgs> const &buffer,
    Expr<uint2> block_size,
    Expr<uint2> dispatch_size,
    Expr<uint> kernel_id);
LC_DSL_API void emplace_indirect_dispatch_kernel3d(
    BufferVar<DispatchIndirectArgs> const &buffer,
    Expr<uint3> block_size,
    Expr<uint3> dispatch_size,
    Expr<uint> kernel_id);
namespace detail {
template<size_t N>
struct UIntType {
    using type = Vector<uint, N>;
};
template<>
struct UIntType<1> {
    using type = uint;
};
}// namespace detail
template<size_t N, typename... Args>
inline void emplace_indirect_dispatch_kernel(
    BufferVar<DispatchIndirectArgs> const &buffer, Kernel<N, Args...> const &kernel, Expr<typename detail::UIntType<N>::type> dispatch_size, Expr<uint> kernel_id) {
    if constexpr (N == 1) {
        emplace_indirect_dispatch_kernel1d(
            buffer,
            kernel.function()->block_size().x,
            dispatch_size,
            kernel_id);
    } else if constexpr (N == 2) {
        emplace_indirect_dispatch_kernel2d(
            buffer,
            kernel.function()->block_size().xy(),
            dispatch_size,
            kernel_id);
    } else {
        emplace_indirect_dispatch_kernel3d(
            buffer,
            kernel.function()->block_size(),
            dispatch_size,
            kernel_id);
    }
    
}
}// namespace luisa::compute