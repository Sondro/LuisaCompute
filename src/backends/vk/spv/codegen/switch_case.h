#pragma once
#include "types.h"
#include <vstl/functional.h>
#include <vstl/small_vector.h>
namespace toolhub::spv {
class SwitchCase : public Component {
public:
	vstd::small_vector<int32> cases;
	vstd::small_vector<Id> caseId;
	Id mergeBlock;
	Id defaultBlock;

	SwitchCase(
		Builder* bd, Id condition,
		vstd::span<int32 const> cases,
		bool containDefault);
	~SwitchCase();
};
}// namespace toolhub::spv