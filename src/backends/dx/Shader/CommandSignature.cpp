#include "CommandSignature.h"
#include <DXRuntime/Device.h>
namespace toolhub::directx {
CommandSignature::CommandSignature(Device* device) {
	D3D12_COMMAND_SIGNATURE_DESC desc;
	D3D12_INDIRECT_ARGUMENT_DESC indDesc;
	memset(&desc, 0, sizeof(D3D12_COMMAND_SIGNATURE_DESC));
	memset(&indDesc, 0, sizeof(D3D12_INDIRECT_ARGUMENT_DESC));
	indDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
	desc.ByteStride = sizeof(D3D12_DISPATCH_ARGUMENTS);
	desc.NodeMask = 0;
	desc.NumArgumentDescs = 1;
	desc.pArgumentDescs = &indDesc;
	ThrowIfFailed(device->device->CreateCommandSignature(&desc, nullptr, IID_PPV_ARGS(&mCommandSignature)));
}
}// namespace toolhub::directx