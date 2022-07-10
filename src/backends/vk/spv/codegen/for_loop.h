#pragma once
#include "block.h"
#include "while_loop.h"
#include <vstl/functional.h>
namespace toolhub::spv {
class ForLoop : public WhileLoop {
public:
	Id afterLoopBlock;
	ForLoop(Builder* bd,
			vstd::move_only_func<Id()> const& condition);
	~ForLoop();
	virtual Id ContinueBlock() const override{ return afterLoopBlock; }
};
}// namespace toolhub::spv