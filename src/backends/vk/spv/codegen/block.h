#pragma once
#include "types.h"
namespace toolhub::spv {
class Block : public Component {
	Id selfId;
	Id jumpId;

public:
	Id SelfId() const { return selfId; }
	Block(Builder* bd, Id selfId, Id jumpId = Id());
	Block(Block&&) = delete;
	Block(Block const&) = delete;
	~Block();
};
}// namespace toolhub::spv