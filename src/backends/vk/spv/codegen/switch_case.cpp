#include "switch_case.h"
#include "builder.h"
namespace toolhub::spv {
SwitchCase::SwitchCase(Builder* bd, Id condition, vstd::span<int32 const> cases)
	: Component(bd), cases(cases) {
	mergeBlock = bd->NewId();
	bd->result << "OpSelectionMerge "sv << mergeBlock.ToString() << " None\nOpSwitch "sv << condition.ToString() << ' ' << mergeBlock.ToString() << ' ';
	caseId.push_back_func(
		cases.size(),
		[&](size_t i) {
			auto id = bd->NewId();
			bd->result << vstd::to_string(cases[i]) << ' ' << id.ToString() << ' ';
			return id;
		});
	bd->result << '\n';
	bd->inBlock = false;
}
void SwitchCase::IterateCase(
	vstd::move_only_func<void(
		size_t index,
		int32 caseValue,
		Id caseId)> const& iterateCase) {
	for (auto i : vstd::range(cases.size())) {
		if (bd->inBlock) {
			bd->inBlock = false;
			bd->result << "OpBranch "sv << caseId[i].ToString() << '\n';
		}
		bd->result << caseId[i].ToString() << " = OpLabel\n"sv;
		bd->inBlock = true;
		iterateCase(
			i,
			cases[i],
			caseId[i]);
		if (bd->inBlock) {
			bd->inBlock = false;
			bd->result << "OpBranch "sv << mergeBlock.ToString() << '\n';
		}
	}
}
SwitchCase::~SwitchCase() {
	if (bd->inBlock) {
		bd->inBlock = false;
		bd->result << "OpBranch "sv << mergeBlock.ToString() << '\n';
	}
	bd->result << mergeBlock.ToString() << " = OpLabel\n"sv;
	bd->inBlock = true;
}
}// namespace toolhub::spv