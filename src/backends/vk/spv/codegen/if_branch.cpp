#include "if_branch.h"
#include "builder.h"
namespace toolhub::spv {
IfBranch::IfBranch(Builder* builder, Id condition)
	: Component(builder), trueBranch(builder->NewId()), falseBranch(builder->NewId()), mergeBlock(builder->NewId()) {
	builder->result << "OpSelectionMerge "sv << mergeBlock.ToString() << " None\nOpBranchConditional "sv
					<< condition.ToString() << ' ' << trueBranch.ToString() << ' ' << falseBranch.ToString() << '\n';
	bd->inBlock = false;
}
IfBranch::~IfBranch() {
	if (bd->inBlock) {
		bd->result << "OpBranch "sv << mergeBlock.ToString() << '\n';
	}
	bd->result << mergeBlock.ToString() << " = OpLabel\n"sv;
	bd->inBlock = true;
}
IfBranch::Branch::Branch(IfBranch* self, Id id) : self(self), id(id) {
	if (self->bd->inBlock) {
		self->bd->result << "OpBranch "sv << id.ToString() << '\n';
	}
	self->bd->result << id.ToString() << " = OpLabel\n"sv;
	self->bd->inBlock = true;
}
IfBranch::Branch::~Branch() {
	if (self->bd->inBlock) {
		self->bd->result << "OpBranch "sv << self->mergeBlock.ToString() << '\n';
		self->bd->inBlock = false;
	}
}
IfBranch::Branch IfBranch::TrueBranchScope() {
	return Branch(this, trueBranch);
}
IfBranch::Branch IfBranch::FalseBranchScope() {
	return Branch(this, falseBranch);
}
}// namespace toolhub::spv