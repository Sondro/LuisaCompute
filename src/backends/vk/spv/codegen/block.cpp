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
Block::~Block() {
	if (bd->inBlock && jumpId.valid()) {
		bd->Str() << "OpBranch "sv << jumpId.ToString() << '\n';
		bd->inBlock = false;
	}
}
}// namespace toolhub::spv