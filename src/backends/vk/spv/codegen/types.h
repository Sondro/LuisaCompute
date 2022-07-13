#pragma once
#include <vstl/Common.h>
#include <ast/type.h>
#include <ast/expression.h>
using namespace luisa;
using namespace luisa::compute;
namespace toolhub::spv {
class Builder;
enum class PointerUsage : vbyte {
	NotPointer = 0,
	Uniform = 1,
	Input = 2,
	Output = 3,
	Workgroup = 4,
	Function = 5,
	UniformConstant = 6
};
using ConstValue = luisa::compute::detail::make_literal_value_t<basic_types>;
struct Id : public vstd::IOperatorNewBase {
	uint id;
	vstd::string ToString() const {
		vstd::string s;
		s << '%';
		vstd::to_string(id, s);
		return s;
	}
	constexpr bool valid() const {
		return id != std::numeric_limits<uint>::max();
	}
	constexpr explicit Id(uint id = std::numeric_limits<uint>::max()) : id(id) {}
	static constexpr Id FloatId() { return Id(0); }
	static constexpr Id IntId() { return Id(1); }
	static constexpr Id UIntId() { return Id(2); }
	static constexpr Id BoolId() { return Id(3); }
	static constexpr Id VecId(Id eleId, uint dim) {
		return Id(4 + 3 * eleId.id + dim - 2);
	}
	static constexpr Id MatId(uint dim) {
		return Id(16 + dim - 2);
	}
	static constexpr Id TrueId() {
		return Id(19);
	}
	static constexpr Id FalseId() {
		return Id(20);
	}
	static constexpr Id ZeroId() {
		return Id(21);
	}
	static constexpr Id VoidId() {
		return Id(22);
	}
	static constexpr Id StartId() {
		return Id(50);
	}
	static constexpr Id RayQueryId() {
		return Id(23);
	}
	static constexpr Id DispatchThreadId() {
		return Id(24);
	}
	static constexpr Id GroupThreadId() {
		return Id(25);
	}
	static constexpr Id GroupId() {
		return Id(26);
	}
	static constexpr Id RayQueryPtrId() {
		return Id(27);
	}
	static constexpr Id AccelId() {
		return Id(28);
	}
	static constexpr Id AccelUniformConstId(){
		return Id(29);
	}
	static constexpr Id AccelVarId(){
		return Id(30);
	}
	static constexpr Id SamplerId(){
		return Id(31);
	}
	static constexpr Id SamplerArrayId(){
		return Id(32);
	}
	static constexpr Id SamplerPtrId(){
		return Id(33);
	}
	static constexpr Id MainId(){
		return Id(48);
	}
	static constexpr Id BuiltinFuncId() {
		return Id(49);
	}
};

struct Component : public vstd::IOperatorNewBase {
protected:
	Builder* bd;

public:
	Component(Builder* bd);
	virtual ~Component() = default;
};
}// namespace toolhub::spv