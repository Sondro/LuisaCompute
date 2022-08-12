#pragma once
#include "types.h"
namespace toolhub::spv {
class IBreakable {
public:
	virtual Id BreakBranch() const { assert(false); }
	virtual Id ContinueBranch() const { assert(false); }
};
}// namespace toolhub::spv