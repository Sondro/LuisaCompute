#pragma once
#include <d3dx12.h>
#include <vstl/PoolAllocator.h>
#include <Resource/BufferView.h>
#include <vstl/VGuid.h>
#include <dxgi1_4.h>
#include <runtime/context.h>
class ElementAllocator;
using Microsoft::WRL::ComPtr;
using namespace luisa::compute;
namespace toolhub::directx {
class IGpuAllocator;
class DescriptorHeap;
class ComputeShader;
class PipelineLibrary;
class DXShaderCompiler;
class Device {

public:
    Context const &ctx;
    struct LazyLoadShader {
    public:
        using LoadFunc = vstd::funcPtr_t<ComputeShader *(Device *, Context const &)>;

    private:
        vstd::unique_ptr<ComputeShader> shader;
        LoadFunc loadFunc;

    public:
        LazyLoadShader(LoadFunc loadFunc);
        ComputeShader *Get(Device *self);
        ~LazyLoadShader();
    };

    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
    Microsoft::WRL::ComPtr<ID3D12Device5> device;
    Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
    vstd::unique_ptr<IGpuAllocator> defaultAllocator;
    vstd::unique_ptr<DescriptorHeap> globalHeap;
    vstd::unique_ptr<DescriptorHeap> samplerHeap;

    LazyLoadShader setAccelKernel;

    LazyLoadShader bc6TryModeG10;
    LazyLoadShader bc6TryModeLE10;
    LazyLoadShader bc6EncodeBlock;

    LazyLoadShader bc7TryMode456;
    LazyLoadShader bc7TryMode137;
    LazyLoadShader bc7TryMode02;
    LazyLoadShader bc7EncodeBlock;
    /*vstd::unique_ptr<ComputeShader> bc6_0;
    vstd::unique_ptr<ComputeShader> bc6_1;
    vstd::unique_ptr<ComputeShader> bc6_2;
    vstd::unique_ptr<ComputeShader> bc7_0;
    vstd::unique_ptr<ComputeShader> bc7_1;
    vstd::unique_ptr<ComputeShader> bc7_2;
    vstd::unique_ptr<ComputeShader> bc7_3;*/
    Device(Context const &cachePath, uint index = 0);
    Device(Device const &) = delete;
    Device(Device &&) = delete;
    ~Device();
    void WaitFence(ID3D12Fence *fence, uint64 fenceIndex);
    static DXShaderCompiler *Compiler();
};
}// namespace toolhub::directx