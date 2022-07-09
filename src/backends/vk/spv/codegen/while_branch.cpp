#include "while_branch.h"
#include "builder.h"
namespace toolhub::spv {
WhileBranch::WhileBranch(
	Builder* bd,
	vstd::move_only_func<Id()> const& condition)
	: Component(bd),
	  conditionBlock(bd->NewId()),
	  loopBlock(bd->NewId()),
	  mergeBlock(bd->NewId()) {
	auto condBlockStr = conditionBlock.ToString();
	if (bd->inBlock) {
		bd->result << "OpBranch "sv << condBlockStr << '\n';
	}
	bd->inBlock = true;
	bd->result << condBlockStr << " = OpLabel"sv << '\n';
	auto condStr = condition().ToString();
	auto mergeStr = mergeBlock.ToString();
	auto loopStr = loopBlock.ToString();
	bd->result << "OpLoopMerge "sv << mergeStr << ' '
			   << loopStr << " None\nOpBranchConditional "sv
			   << condStr << ' ' << loopStr << ' ' << mergeStr << '\n'
			   << loopStr << " = OpLabel\n"sv;
}
WhileBranch::~WhileBranch() {
	if (bd->inBlock) {
		bd->result << "OpBranch "sv << conditionBlock.ToString() << '\n';
	}
	bd->result << mergeBlock.ToString() << " = OpLabel\n"sv;
	bd->inBlock = true;
}
}// namespace toolhub::spv