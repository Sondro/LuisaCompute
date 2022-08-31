#pragma once
#include <core/stl/vector.h>
#include <core/stl/functional.h>
namespace luisa::compute {
class SocketVisitor {
protected:
	~SocketVisitor() = default;

public:
	using WaitSend = luisa::move_only_function<void()>;
	using WaitReceive = luisa::move_only_function<luisa::vector<std::byte>()>;
	virtual WaitSend send_async(luisa::vector<std::byte>&& src) = 0;
	virtual WaitReceive receive_async() = 0;
};
}// namespace luisa::compute