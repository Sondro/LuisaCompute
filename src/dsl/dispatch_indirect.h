#pragma once
#include <dsl/expr.h>
#include <dsl/var.h>
#include <dsl/struct.h>
#include <dsl/dispatch_indirect_decl.h>
LUISA_CUSTOM_STRUCT(DispatchIndirectArgs);
namespace luisa::compute {
inline void clear_dispatch_indirect_buffer(BufferVar<DispatchIndirectArgs> const &buffer) {
    detail::FunctionBuilder::current()->call(CallOp::CLEAR_DISPATCH_INDIRECT_BUFFER, {buffer.expression()});
}
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
    BufferVar<DispatchIndirectArgs> const &buffer, Kernel<N, Args...> const &kernel, Expr<typename detail::UIntType<N>::type> const &dispatch_size, Expr<uint> const &kernel_id) {
    auto expr = [&] {
        if constexpr (N == 1) {
            return def<uint>(kernel.function()->block_size().x).expression();
        } else if constexpr (N == 2) {
            return def<uint2>(kernel.function()->block_size().xy()).expression();
        } else {
            return def<uint3>(kernel.function()->block_size()).expression();
        }
    }();
    detail::FunctionBuilder::current()->call(
        CallOp::EMPLACE_DISPATCH_INDIRECT_KERNEL,
        {buffer.expression(),
         expr,
         dispatch_size.expression(), kernel_id.expression()});
}
}// namespace luisa::compute