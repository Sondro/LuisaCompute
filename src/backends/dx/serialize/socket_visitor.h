#pragma once
#include <core/stl/vector.h>
namespace luisa::compute {
class SocketVisitor {
protected:
	~SocketVisitor() = default;

public:
    virtual void send(luisa::span<std::byte const> data) = 0;
    virtual luisa::vector<std::byte> receive() = 0;
};
}// namespace luisa::compute