#pragma once
#include "types.h"
#include <vstl/functional.h>
namespace toolhub::spv {
class WhileBranch : public Component {
public:
    Id conditionBlock;
    Id loopBlock;
    Id mergeBlock;
	WhileBranch(
        Builder* bd,
        vstd::move_only_func<Id()> const& condition);
    ~WhileBranch();
};
}// namespace toolhub::spv