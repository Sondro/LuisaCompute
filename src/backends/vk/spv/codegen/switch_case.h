#pragma once
#include "types.h"
#include <vstl/functional.h>
#include <vstl/small_vector.h>
namespace toolhub::spv {
class SwitchCase : public Component {
	vstd::span<int32 const> cases;
	vstd::small_vector<Id> caseId;
	Id mergeBlock;

public:
	SwitchCase(Builder* bd, Id condition, vstd::span<int32 const> cases);
	void IterateCase(
		vstd::move_only_func<void(
			size_t index,
			int32 caseValue,
			Id caseId)> const& iterateCase);
	~SwitchCase();
};
}// namespace toolhub::spv