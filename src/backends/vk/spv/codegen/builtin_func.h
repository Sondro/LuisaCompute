#pragma once
#include "types.h"
#include <ast/function.h>
#include "variable.h"
namespace toolhub::spv {
class BuiltinFunc : public Component {
	template<typename Ite>
	requires(vstd::IterableType<Ite>)
	Id FuncCall(Id type, vstd::string_view name, Ite&& args);
	template<typename Ite>
	requires(vstd::IterableType<Ite>)
	Id OpCall(Id type, vstd::string_view name, Ite&& args);
	Id OpCall(Id type, vstd::string_view name, Id arg);

public:
	BuiltinFunc(Builder* builder);
	Id CallFunc(
		luisa::compute::Type const* callType,
		CallOp op,
		vstd::IRange<Variable>& arg);
};
}// namespace toolhub::spv