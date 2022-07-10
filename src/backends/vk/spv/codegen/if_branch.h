#pragma once
#include "block.h"
namespace toolhub::spv {
class Builder;
class IfBranch : public Component {
public:
	Id mergeBlock;
	Id trueBranch;
	Id falseBranch;
	IfBranch(Builder* builder, Id condition);
	~IfBranch();
};
}// namespace toolhub::spv