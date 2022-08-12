#include "while_loop.h"
#include "builder.h"
namespace toolhub::spv {
WhileLoop::WhileLoop(
	Builder* bd,
	vstd::function<void()>&& afterLoopFunc,
	vstd::function<Id()> const& getCondition)
	: Component(bd),
	  afterLoopFunc(std::move(afterLoopFunc)),
	  beforeLoopBranch{bd->NewId()},
	  loopBranch{bd->NewId()},
	  continueBranch{bd->NewId()},
	  breakBranch{bd->NewId()} {
	if (bd->inBlock) {
		bd->Str() << "OpBranch "sv << beforeLoopBranch.ToString() << '\n';
	}
	bd->inBlock = true;
	bd->Str() << beforeLoopBranch.ToString() << " = OpLabel\n"sv;
	auto condStr = getCondition().ToString();
	bd->Str() << "OpLoopMerge "sv << breakBranch.ToString() << ' ' << continueBranch.ToString() << " None\n"sv;

	/*
%init_loop = OpLabel
OpLoopMerge %finish_loop %continue_loop None
OpBranch %start_loop
%start_loop = OpLabel
OpBranch %goto_break

%goto_break = OpLabel
OpBranch %finish_loop

%continue_loop = OpLabel
OpBranch %init_loop

%finish_loop = OpLabel
OpBranch %finish_func

%finish_func = OpLabel
OpReturn
OpFunctionEnd
 */
}

WhileLoop::~WhileLoop() {
	{
		auto builder = bd->Str();
		if (bd->inBlock) {
			builder << "OpBranch "sv << continueBranch.ToString() << '\n';
		}
		builder << continueBranch.ToString() << " = OpLabel\n"sv;
	}
	bd->inBlock = true;
	afterLoopFunc();
	{
		auto builder = bd->Str();
		if (bd->inBlock) {
			builder << "OpBranch "sv << beforeLoopBranch.ToString() << '\n';
		}
		builder << breakBranch.ToString() << " = OpLabel\n"sv;
	}
	bd->inBlock = true;
}
}// namespace toolhub::spv