//
// Created by Mike Smith on 2022/5/23.
//

#pragma once

#include <embree3/rtcore_device.h>
#include <runtime/device.h>

namespace llvm {
class TargetMachine;
}

namespace llvm::orc {
class LLJIT;
}

namespace luisa::compute::llvm {

class LLVMDevice : public Device::Interface {

private:
    RTCDevice _rtc_device;
    std::unique_ptr<::llvm::TargetMachine> _target_machine;
    mutable std::unique_ptr<::llvm::orc::LLJIT> _jit;
    mutable std::mutex _jit_mutex;

public:
    explicit LLVMDevice(const Context &ctx) noexcept;
    ~LLVMDevice() noexcept override;
    [[nodiscard]] ::llvm::TargetMachine *target_machine() const noexcept { return _target_machine.get(); }
    [[nodiscard]] ::llvm::orc::LLJIT *jit() const noexcept { return _jit.get(); }
    [[nodiscard]] auto &jit_mutex() const noexcept { return _jit_mutex; }
    void *native_handle() const noexcept override;
    uint64_t create_buffer(size_t size_bytes) noexcept override;
    void destroy_buffer(uint64_t handle) noexcept override;
    void *buffer_native_handle(uint64_t handle) const noexcept override;
    uint64_t create_texture(PixelFormat format, uint dimension, uint width, uint height, uint depth, uint mipmap_levels) noexcept override;
    void destroy_texture(uint64_t handle) noexcept override;
    void *texture_native_handle(uint64_t handle) const noexcept override;
    uint64_t create_bindless_array(size_t size) noexcept override;
    void destroy_bindless_array(uint64_t handle) noexcept override;
    void emplace_buffer_in_bindless_array(uint64_t array, size_t index, uint64_t handle, size_t offset_bytes) noexcept override;
    void emplace_tex2d_in_bindless_array(uint64_t array, size_t index, uint64_t handle, Sampler sampler) noexcept override;
    void emplace_tex3d_in_bindless_array(uint64_t array, size_t index, uint64_t handle, Sampler sampler) noexcept override;
    bool is_resource_in_bindless_array(uint64_t array, uint64_t handle) const noexcept override;
    void remove_buffer_in_bindless_array(uint64_t array, size_t index) noexcept override;
    void remove_tex2d_in_bindless_array(uint64_t array, size_t index) noexcept override;
    void remove_tex3d_in_bindless_array(uint64_t array, size_t index) noexcept override;
    uint64_t create_stream(bool for_present) noexcept override;
    void destroy_stream(uint64_t handle) noexcept override;
    void synchronize_stream(uint64_t stream_handle) noexcept override;
    void dispatch(uint64_t stream_handle, const CommandList &list) noexcept override;
    void dispatch(uint64_t stream_handle, move_only_function<void()> &&func) noexcept override;
    void *stream_native_handle(uint64_t handle) const noexcept override;
    uint64_t create_swap_chain(uint64_t window_handle, uint64_t stream_handle, uint width, uint height, bool allow_hdr, uint back_buffer_size) noexcept override;
    void destroy_swap_chain(uint64_t handle) noexcept override;
    PixelStorage swap_chain_pixel_storage(uint64_t handle) noexcept override;
    void present_display_in_stream(uint64_t stream_handle, uint64_t swapchain_handle, uint64_t image_handle) noexcept override;
    uint64_t create_shader(Function kernel, std::string_view meta_options) noexcept override;
    void destroy_shader(uint64_t handle) noexcept override;
    uint64_t create_event() noexcept override;
    void destroy_event(uint64_t handle) noexcept override;
    void signal_event(uint64_t handle, uint64_t stream_handle) noexcept override;
    void wait_event(uint64_t handle, uint64_t stream_handle) noexcept override;
    void synchronize_event(uint64_t handle) noexcept override;
    uint64_t create_mesh(uint64_t v_buffer, size_t v_offset, size_t v_stride, size_t v_count, uint64_t t_buffer, size_t t_offset, size_t t_count, AccelUsageHint hint) noexcept override;
    void destroy_mesh(uint64_t handle) noexcept override;
    uint64_t create_accel(AccelUsageHint hint) noexcept override;
    void destroy_accel(uint64_t handle) noexcept override;
};

}// namespace luisa::compute::llvm

