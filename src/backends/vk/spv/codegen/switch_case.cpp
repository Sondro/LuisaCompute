#include "switch_case.h"
#include "builder.h"
namespace toolhub::spv {
SwitchCase::SwitchCase(
	Builder* bd,
	Id condition,
	vstd::span<int32 const> cases,
	bool containDefault)
	: Component(bd),
	  cases(cases),
	  mergeBlock(bd->NewId()),
	  defaultBlock(containDefault ? bd->NewId() : Id()) {
	bd->Str() << "OpSelectionMerge "sv << mergeBlock.ToString() << " None\nOpSwitch "sv << condition.ToString() << ' ' << (containDefault ? defaultBlock : mergeBlock).ToString() << ' ';
	caseId.push_back_func(
		cases.size(),
		[&](size_t i) {
			auto id = bd->NewId();
			bd->Str() << vstd::to_string(cases[i]) << ' ' << id.ToString() << ' ';
			return id;
		});
	bd->Str() << '\n';
	bd->inBlock = false;
}
SwitchCase::~SwitchCase() {
	if (bd->inBlock) {
		bd->inBlock = false;
		bd->Str() << "OpBranch "sv << mergeBlock.ToString() << '\n';
	}
	bd->Str() << mergeBlock.ToString() << " = OpLabel\n"sv;
	bd->inBlock = true;
}
}// namespace toolhub::spv