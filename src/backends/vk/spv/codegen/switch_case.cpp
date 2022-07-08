#include "switch_case.h"
#include "builder.h"
namespace toolhub::spv {
SwitchCase::SwitchCase(Builder* bd, Id condition, vstd::span<int32> cases)
	: Component(bd), cases(cases) {
	mergeBlock = bd->NewId();
	bd->result << "OpSelectionMerge "_sv << mergeBlock.ToString() << " None\nOpSwitch "_sv << condition.ToString() << ' ' << mergeBlock.ToString() << ' ';
	caseId.push_back_func(
		cases.size(),
		[&](size_t i) {
			auto id = bd->NewId();
			bd->result << vstd::to_string(cases[i]) << ' ' << id.ToString() << '\n';
			return id;
		});
	bd->result << '\n';
}
void SwitchCase::IterateCase(
	vstd::move_only_func<void(
		size_t index,
		int32 caseValue,
		Id caseId)> const& iterateCase) {
	for (auto i : vstd::range(cases.size())) {
		bd->result << caseId[i].ToString() << " = OpLabel\n"_sv;
		iterateCase(
			i,
			cases[i],
			caseId[i]);
		bd->result << "OpBranch "_sv << mergeBlock.ToString();
	}
}
SwitchCase::~SwitchCase() {
	bd->result << mergeBlock.ToString() << " = OpLabel\n"_sv;
}
}// namespace toolhub::spv