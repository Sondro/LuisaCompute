#pragma once
#include <vstl/Common.h>
#include <runtime/util.h>
using namespace luisa::compute;
namespace toolhub::directx {
class Device;
class LCUtil : public IUtil, public vstd::IOperatorNewBase {
public:
    static constexpr size_t BLOCK_SIZE = 16;
    Device *device;
    LCUtil(Device *device);
    ~LCUtil();
    Result compress_bc6h(Stream &stream, Image<float> const &src, luisa::vector<std::byte> &result) noexcept override;
    Result compress_bc7(Stream &stream, Image<float> const &src, luisa::vector<std::byte> &result, float alphaImportance) noexcept override;
};
}// namespace toolhub::directx