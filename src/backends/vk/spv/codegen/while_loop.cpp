#include "while_loop.h"
#include "builder.h"
namespace toolhub::spv {
LoopBase::LoopBase(Builder* bd)
	: conditionBlock(bd->NewId()),
	  loopBlock(bd->NewId()),
	  mergeBlock(bd->NewId()) {
}

WhileLoop::WhileLoop(
	Builder* bd,
	vstd::function<Id()> const& condition)
	: Component(bd),
	  LoopBase(bd) {
	auto condBlockStr = conditionBlock.ToString();
	if (bd->inBlock) {
		bd->Str() << "OpBranch "sv << condBlockStr << '\n';
		bd->inBlock = false;
	}
	bd->Str() << condBlockStr << " = OpLabel"sv << '\n';
	auto condStr = condition().ToString();
	auto mergeStr = mergeBlock.ToString();
	auto loopStr = loopBlock.ToString();
	bd->Str() << "OpLoopMerge "sv << mergeStr << ' '
			   << loopStr << " None\nOpBranchConditional "sv
			   << condStr << ' ' << loopStr << ' ' << mergeStr << '\n';
}
WhileLoop::~WhileLoop() {
	if (bd->inBlock) {
		bd->Str() << "OpBranch "sv << mergeBlock.ToString() << '\n';
	}
	bd->Str() << mergeBlock.ToString() << " = OpLabel\n"sv;
	bd->inBlock = true;
}
}// namespace toolhub::spv