#include "function.h"
#include "builder.h"
namespace toolhub::spv {
Function::Function(Builder* builder, Id returnType)
	: Component(builder),returnType(returnType), funcType(builder->GetFuncReturnTypeId(returnType)), func(builder->NewId()) {
	bd->result << func.ToString() << " = OpFunction "_sv << returnType.ToString() << " None "_sv << funcType.ToString() << '\n';
}
Function::~Function() {
	if (func.id == Id::VoidId().id)
		bd->result << "OpReturn\n"_sv;
	bd->result << "OpFunctionEnd\n"_sv;
}
}// namespace toolhub::spv