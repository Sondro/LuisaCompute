#include "function.h"
#include "builder.h"
namespace toolhub::spv {
Function::Function(Builder* builder, Id returnType, vstd::span<Id> argType)
	: Component(builder), funcBlockId(builder->NewId()), returnType(returnType), funcType(builder->GetFuncReturnTypeId(returnType, argType)), func(builder->NewId()) {
	bd->result << func.ToString() << " = OpFunction "_sv << returnType.ToString() << " None "_sv << funcType.ToString() << '\n';
	argValues.push_back_func(
		argType.size(),
		[&](size_t i) {
			Id newId(bd->NewId());
			bd->result << newId.ToString() << " = OpFunctionParameter "_sv << argType[i].ToString() << '\n';
			return newId;
		});
	bd->result << funcBlockId.ToString() << " = OpLabel\n"_sv;
}
Function::~Function() {
	if (func.id == Id::VoidId().id)
		bd->result << "OpReturn\n"_sv;
	bd->result << "OpFunctionEnd\n"_sv;
}
}// namespace toolhub::spv