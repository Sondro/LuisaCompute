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
	bd->result << conditionBlock.ToString() << " = OpLabel"_sv << '\n';
	auto condStr = condition().ToString();
	auto mergeStr = mergeBlock.ToString();
	auto loopStr = loopBlock.ToString();
	bd->result << "OpLoopMerge "_sv << mergeStr << ' '
			   << loopStr << " None\nOpBranchConditional "_sv
			   << condStr << ' ' << loopStr << mergeStr << '\n'
               << loopStr << " = OpLabel\n"_sv;
}
WhileBranch::~WhileBranch() {
    bd->result << "OpBranch "_sv << conditionBlock.ToString() << '\n' << mergeBlock.ToString() << " = OpLabel\n"_sv;
}
}// namespace toolhub::spv