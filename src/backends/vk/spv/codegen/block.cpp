#include "block.h"
#include "builder.h"
namespace toolhub::spv {
Block::Block(Builder* bd, Id selfId, Id jumpId)
	: Component(bd), selfId(selfId), jumpId(jumpId) {
	if (bd->inBlock) {
		bd->Str() << "OpBranch "sv << selfId.ToString() << '\n';
	}
	bd->Str() << selfId.ToString() << " = OpLabel\n"sv;
	bd->inBlock = true;
}
Block::Block(Block&& v)
	: Component(std::move(v)), selfId(v.selfId), jumpId(v.jumpId) {
	v.selfId = Id{};
	v.jumpId = Id{};
}

Block::~Block() {
	if (jumpId.valid() && bd->inBlock) {
		bd->Str() << "OpBranch "sv << jumpId.ToString() << '\n';
		bd->inBlock = false;
	}
}
}// namespace toolhub::spv