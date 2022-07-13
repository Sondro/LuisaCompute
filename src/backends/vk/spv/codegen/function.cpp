#include "function.h"
#include "builder.h"
namespace toolhub::spv {
Function::Function(Builder* builder, Id returnType, vstd::span<Id const> argType)
	: Component(builder), funcBlockId(builder->NewId()), returnType(returnType), funcType(builder->GetFuncReturnTypeId(returnType, argType)), func(builder->NewId()) {
	bd->Str() << func.ToString() << " = OpFunction "sv << returnType.ToString() << " None "sv << funcType.ToString() << '\n';
	argValues.push_back_func(
		argType.size(),
		[&](size_t i) {
			Id newId(bd->NewId());
			bd->Str() << newId.ToString() << " = OpFunctionParameter "sv << argType[i].ToString() << '\n';
			return newId;
		});
}
Function::Function(Builder* builder)
	: Component(builder),
	  returnType(Id::VoidId()),
	  funcType(bd->GetFuncReturnTypeId(Id::VoidId(), {})), funcBlockId(builder->NewId()) {
	bd->Str() << "%48 = OpFunction %22 None "sv << funcType.ToString() << '\n';
}
Id Function::GetReturnTypeBranch(Id value) {
	auto id = returnValues.emplace_back(Id(bd->NewId()), value).first;
	return id;
}
Id Function::GetReturnBranch() {
	if (!returnVoidCmd.valid())
		returnVoidCmd = bd->NewId();
	return returnVoidCmd;
}

Function::~Function() {
	if (returnValues.empty()) {
		if (returnVoidCmd.valid()) {
			if (bd->inBlock) {
				bd->Str() << "OpBranch "sv << returnVoidCmd.ToString() << '\n';
			}
			bd->Str() << returnVoidCmd.ToString() << " = OpLabel\nOpReturn\n"sv;
		} else {
			bd->Str() << "OpReturn\n"sv;
		}
	} else {
		if (bd->inBlock) {
			bd->Str() << "OpBranch "sv << returnValues[0].first.ToString() << '\n';
		}
		auto ite = [&](auto&& pair) {
			bd->Str() << pair.first.ToString() << " = OpLabel\nOpReturnValue "sv << pair.second.ToString() << '\n';
		};
		ite(returnValues[0]);
		for (auto i : vstd::range(1, returnValues.size())) {
			ite(returnValues[1]);
		}
	}
	bd->inBlock = false;
	bd->Str() << "OpFunctionEnd\n"sv;
}
}// namespace toolhub::spv