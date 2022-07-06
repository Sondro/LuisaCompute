#include "if_branch.h"
#include "builder.h"
namespace toolhub::spv {
IfBranch::IfBranch(Builder* builder, Id condition)
	: Component(builder), trueBranch(builder->NewId()), falseBranch(builder->NewId()) , mergeBlock(builder->NewId()){
	builder->result << "OpSelectionMerge "_sv << mergeBlock.ToString() << " None\nOpBranchConditional "_sv
					<< condition.ToString() << ' ' << trueBranch.ToString() << ' ' << falseBranch.ToString();
}
IfBranch::~IfBranch() {
	bd->result << mergeBlock.ToString() << " = OpLabel"_sv;
}
IfBranch::Branch::Branch(IfBranch* self, Id id) : self(self), id(id) {
	self->bd->result << id.ToString() << " = OpLabel\n"_sv;
}
IfBranch::Branch::~Branch() {
	self->bd->result << "OpBranch "_sv << self->mergeBlock.ToString() << '\n';
}
IfBranch::Branch IfBranch::TrueBranchScope() {
    return Branch(this, trueBranch);
}
IfBranch::Branch IfBranch::FalseBranchScoe() {
    return Branch(this, falseBranch);
}
}// namespace toolhub::spv