#include "function.h"
#include "builder.h"
namespace toolhub::spv {
Function::Function(Builder* builder, Id returnType, vstd::span<Id const> argType)
	: Component(builder), funcBlockId(builder->NewId()), returnType(returnType), funcType(builder->GetFuncReturnTypeId(returnType, argType)), func(builder->NewId()) {
	bd->result << func.ToString() << " = OpFunction "sv << returnType.ToString() << " None "sv << funcType.ToString() << '\n';
	argValues.push_back_func(
		argType.size(),
		[&](size_t i) {
			Id newId(bd->NewId());
			bd->result << newId.ToString() << " = OpFunctionParameter "sv << argType[i].ToString() << '\n';
			return newId;
		});
	bd->result << funcBlockId.ToString() << " = OpLabel\n"sv;
	bd->inBlock = true;
}
Function::Function(Builder* builder)
	: Component(builder),
	  returnType(Id::VoidId()),
	  funcType(bd->GetFuncReturnTypeId(Id::VoidId(), {})), funcBlockId(builder->NewId()) {
	bd->result << "%main = OpFunction %22 None "sv << funcType.ToString() << '\n'
			   << funcBlockId.ToString() << " = OpLabel\n"sv;
	bd->inBlock = true;
}
Id Function::GetReturnTypeBranch(Id value) {
	auto id = returnValues.emplace_back(Id(bd->NewId()), value).first;
	return id;
}

Function::~Function() {
	if (returnValues.empty())
		bd->result << "OpReturn\n"sv;
	else {
		if (bd->inBlock) {
			bd->result << "OpBranch "sv << returnValues[0].first.ToString() << '\n';
		}
		auto ite = [&](auto&& pair) {
			bd->result << pair.first.ToString() << " = OpLabel\nOpReturnValue "sv << pair.second.ToString() << '\n';
		};
		ite(returnValues[0]);
		for (auto i : vstd::range(1, returnValues.size())) {
			ite(returnValues[1]);
		}
	}
	bd->inBlock = false;
	bd->result << "OpFunctionEnd\n"sv;
}
}// namespace toolhub::spv