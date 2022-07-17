#include "for_loop.h"
#include "builder.h"
namespace toolhub::spv {
ForLoop::ForLoop(Builder* bd,
				 vstd::function<Id()> const& condition)
	: WhileLoop(bd, condition),
	  afterLoopBlock(bd->NewId()) {
}
ForLoop::~ForLoop() {
}
}// namespace toolhub::spv