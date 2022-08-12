#include "function.h"
#include "builder.h"
namespace toolhub::spv {
Function::Function(Builder* builder, Id returnType, vstd::IRange<Id>* argType)
	: Component(builder), funcBlockId(builder->NewId()), returnType(returnType), funcType(builder->GetFuncReturnTypeId(returnType, argType)), func(builder->NewId()) {
	{
		auto builder = bd->Str();
		builder << func.ToString() << " = OpFunction "sv << returnType.ToString() << " None "sv << funcType.ToString() << '\n';
		size_t i = 0;
		if (argType) {
			for (auto&& arg : *argType) {
				Id newId(bd->NewId());
				builder << newId.ToString() << " = OpFunctionParameter "sv << arg.ToString() << '\n';
				argValues.emplace_back(newId);
				++i;
			}
		}
	}

	varString = &(builder->strings[builder->stringCount - 2]);
}
Function::Function(Builder* builder)
	: Component(builder),
	  returnType(Id::VoidId()),
	  funcType(bd->GetFuncReturnTypeId(Id::VoidId(), nullptr)), funcBlockId(builder->NewId()) {
	bd->Str() << "%48 = OpFunction %22 None "sv << funcType.ToString() << '\n';

	varString = &(builder->strings[builder->stringCount - 2]);
}
/*
Id Function::GetReturnTypeBranch(Id value) {
	auto id = returnValues.emplace_back(Id(bd->NewId()), value).first;
	return id;
}
Id Function::GetReturnBranch() {
	if (!returnVoidCmd.valid())
		returnVoidCmd = bd->NewId();
	return returnVoidCmd;
}
*/
void Function::ReturnValue(Id returnValue) {
	assert(bd->inBlock && returnType.valid() && returnType != Id::VoidId());
	bd->inBlock = false;
	bd->Str() << "OpReturnValue "sv << returnValue.ToString() << '\n';
	//returnValues.emplace_back(bd->NewId(), returnValue);
}
void Function::Return() {
	assert(bd->inBlock && (!returnType.valid() || returnType == Id::VoidId()));
	bd->inBlock = false;
	bd->Str() << "OpReturn\n"sv;
}

Function::~Function() {
	auto builder = bd->Str();
	if (bd->inBlock) {
		if (!returnType.valid() || returnType == Id::VoidId()) {
			builder << "OpReturn\n"sv;
		} else {
			Id retValue(bd->NewId());
			builder
				<< "OpUnreachable "sv << retValue.ToString() << '\n';
		}
		bd->inBlock = false;
	}
	/*
	if (returnValues.empty()) {
		if (bd->inBlock) {
			bd->Str() << "OpReturn\n"sv;
		}
	} else {
		if (bd->inBlock) {
			bd->Str() << "OpBranch "sv << returnValues[0].first.ToString() << '\n';
		}
		for (auto&& pair : returnValues) {
			bd->Str() << pair.first.ToString() << " = OpLabel\nOpReturnValue "sv << pair.second.ToString() << '\n';
		}
	}*/
	builder << "OpFunctionEnd\n"sv;
}
}// namespace toolhub::spv