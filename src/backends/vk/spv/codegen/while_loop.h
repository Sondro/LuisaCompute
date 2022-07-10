#pragma once
#include "types.h"
#include <vstl/functional.h>
#include "loop_base.h"
namespace toolhub::spv {
class WhileLoop : public Component, public LoopBase {
public:
	WhileLoop(
		Builder* bd,
		vstd::move_only_func<Id()> const& condition);
	virtual ~WhileLoop();
	virtual Id ContinueBlock() const override{ return mergeBlock; }
};
}// namespace toolhub::spv