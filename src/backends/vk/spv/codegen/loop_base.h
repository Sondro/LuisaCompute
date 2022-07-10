#pragma once
#include "types.h"
namespace toolhub::spv {
class LoopBase {
protected:
	Id conditionBlock;
	Id loopBlock;
	Id mergeBlock;

public:
	LoopBase(Builder* bd);
	Id ConditionBlock() const { return conditionBlock; }
	Id LoopBlock() const { return loopBlock; }
	Id MergeBlock() const { return mergeBlock; }
	virtual Id ContinueBlock() const = 0;
};
}// namespace toolhub::spv