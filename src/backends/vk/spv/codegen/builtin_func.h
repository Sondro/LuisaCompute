#pragma once
#include "types.h"
#include <ast/function.h>
#include "variable.h"
namespace toolhub::spv {
class InternalType;
class BuiltinFunc : public Component {
	template<typename Ite>
		requires(vstd::IterableType<Ite>)
	Id FuncCall(Id type, vstd::string_view name, Ite&& args);
	template<typename Ite>
		requires(vstd::IterableType<Ite>)
	Id OpCall(Id type, vstd::string_view name, Ite&& args);
	Id OpCall(Id type, vstd::string_view name, Id arg);
	Id GetPayload(
		Variable& accel,
		Variable& desc,
		uint rayFlag);
	vstd::string_view FuncFUSName(
		InternalType const& type,
		vstd::string_view floatName,
		vstd::string_view intName,
		vstd::string_view uintName);
	vstd::string_view FuncFIName(
		InternalType const& type,
		vstd::string_view floatName,
		vstd::string_view intName);
	Id MakeVector(
		InternalType tarType,
		vstd::IRange<Variable>& arg);

public:
	BuiltinFunc(Builder* builder);
	Id CallFunc(
		luisa::compute::Type const* callType,
		CallOp op,
		vstd::IRange<Variable>& arg);
};
}// namespace toolhub::spv