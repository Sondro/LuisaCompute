#pragma once
#include <core/stl/vector.h>
#include <core/stl/functional.h>
namespace luisa::compute {
class SocketVisitor {
protected:
	~SocketVisitor() = default;

public:
	using WaitReceive = luisa::move_only_function<luisa::vector<std::byte>()>;
	virtual void send_async(luisa::vector<std::byte>&& src) = 0;
	virtual WaitReceive receive_async() = 0;
	virtual void receive(luisa::vector<std::byte>& data) = 0;
};
}// namespace luisa::compute