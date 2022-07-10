#include "if_branch.h"
#include "builder.h"
namespace toolhub::spv {
IfBranch::IfBranch(Builder* builder, Id condition)
	: Component(builder), trueBranch(builder->NewId()), falseBranch(builder->NewId()), mergeBlock(builder->NewId()) {
	builder->Str() << "OpSelectionMerge "sv << mergeBlock.ToString() << " None\nOpBranchConditional "sv
					<< condition.ToString() << ' ' << trueBranch.ToString() << ' ' << falseBranch.ToString() << '\n';
	bd->inBlock = false;
}
IfBranch::~IfBranch() {
	if (bd->inBlock) {
		bd->Str() << "OpBranch "sv << mergeBlock.ToString() << '\n';
	}
	bd->Str() << mergeBlock.ToString() << " = OpLabel\n"sv;
	bd->inBlock = true;
}
}// namespace toolhub::spv