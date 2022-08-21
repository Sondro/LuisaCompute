#pragma once
#include <vstl/Common.h>
#include <d3dx12.h>
using namespace Microsoft::WRL;
namespace toolhub::directx {
class Device;
class CommandSignature : public vstd::IOperatorNewBase {
public:
	ComPtr<ID3D12CommandSignature> mCommandSignature;
	CommandSignature(Device* device);
	ID3D12CommandSignature* GetSignature() const { return mCommandSignature.Get(); }
};
}// namespace toolhub::directx