//
// Created by Mike Smith on 2021/6/23.
//

#include <iostream>

#include <stb/stb_image_write.h>

#include <runtime/context.h>
#include <runtime/device.h>
#include <runtime/stream.h>
#include <runtime/event.h>
#include <dsl/syntax.h>
#include <dsl/sugar.h>
#include <dsl/dispatch_indirect.h>
#include <rtx/accel.h>
#include <vstl/Common.h>

using namespace luisa;
using namespace luisa::compute;

int main(int argc, char *argv[]) {
    /*
    Dispatch 16 times in gpu, result should be:
    i0 = 1 + 2 + ... + 16
    i1 = 2 + 3 + ... + 16
    ...
    i15 = 15 + 16
    i16 = 16
    i17 = 0
    */
    log_level_info();
    Context context{argv[0]};
    auto device = context.create_device("dx");

    auto stream = device.create_stream();
    static constexpr uint block_size = 32;
    Kernel1D accumulate_kernel = [&](BufferVar<uint> buffer) {
        set_block_size(block_size, 1, 1);
        buffer.atomic(dispatch_id().x).fetch_add(kernel_id());
    };
    Kernel1D clear_kernel = [](BufferVar<DispatchArgs1D> ind_buffer) {
        set_block_size(1);
        clear_dispatch_buffer(ind_buffer);
    };
    Kernel1D indirect_kernel = [&](BufferVar<DispatchArgs1D> ind_buffer) {
        set_block_size(16);
        auto index = dispatch_id().x + 1;
        // Same
        // emplace_indirect_dispatch_kernel(ind_buffer, accumulate_kernel, index, index);
        emplace_dispatch_kernel(ind_buffer, block_size, index, index);
    };
    auto accumulate_shader = device.compile(accumulate_kernel, false);
    auto clear_shader = device.compile(clear_kernel, false);
    auto indirect_shader = device.compile(indirect_kernel, false);
    auto indirect_buffer = device.create_1d_dispatch_buffer(2048);// A large capacity
    auto buffer = device.create_buffer<uint>(17);
    uint value[17];
    memset(value, 0, 17 * sizeof(uint));

    stream
        << buffer.copy_from(value)
        << clear_shader(indirect_buffer).dispatch(1)
        << indirect_shader(indirect_buffer).dispatch(16)
        << accumulate_shader(buffer).dispatch(indirect_buffer)
        << buffer.copy_to(value)
        << synchronize();
    std::cout << "result should be: 136 135 133 130 126 121 115 108 100 91 81 70 58 45 31 16 0\n";
    for (auto &&i : value) {
        std::cout << i << ' ';
    }
    std::cout << '\n';
}
