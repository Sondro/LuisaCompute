#pragma once
#include "i_breakable.h"
#include <vstl/functional.h>
namespace toolhub::spv {
class WhileLoop : public Component, public IBreakable {
public:
	Id beforeLoopBranch;
	Id loopBranch;
	Id continueBranch;
	Id breakBranch;
	vstd::function<void()> afterLoopFunc;
	WhileLoop(
		Builder* bd,
		vstd::function<void()>&& afterLoopFunc,
		vstd::function<Id()> const& getCondition);
	Id BreakBranch() const override { return breakBranch; }
	Id ContinueBranch() const override { return continueBranch; }
	virtual ~WhileLoop();
};
}// namespace toolhub::spv