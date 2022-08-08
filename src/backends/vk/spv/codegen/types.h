#pragma once
#include <vstl/Common.h>
#include <ast/type.h>
#include <ast/expression.h>
#include <vstl/ranges.h>
using namespace luisa;
using namespace luisa::compute;
namespace toolhub::spv {
class Builder;
enum class PointerUsage : vbyte {
	NotPointer = 0,
	StorageBuffer = 1,
	Input = 2,
	Output = 3,
	Workgroup = 4,
	Function = 5,
	UniformConstant = 6
};
using ConstValue = luisa::compute::detail::make_literal_value_t<basic_types>;
struct ConstValueHash {
	size_t operator()(ConstValue const& value) const {
		size_t hash = 0;
		luisa::visit(
			[&](auto&& v) {
				const vstd::hash<std::remove_cvref_t<decltype(v)>> hs;
				hash = hs(v);
			},
			value);
		return hash;
	}
};
template<size_t i, typename Dst, typename T, typename... Args>
constexpr size_t GetVariantIndexFunc() {
	if constexpr (std::is_same_v<Dst, T>) {
		return i;
	} else if constexpr (sizeof...(Args) == 0) {
		return i + 1;
	} else {
		return GetVariantIndexFunc<i + 1, Dst, Args...>();
	}
};
template<typename Dst, typename T>
struct GetVariantIndex;
template<typename Dst, typename T, typename... Args>
struct GetVariantIndex<Dst, luisa::variant<T, Args...>> {
	static constexpr size_t value = GetVariantIndexFunc<0, Dst, T, Args...>();
	static constexpr bool valid_type = value < (sizeof...(Args) + 1);
};

struct ConstValueCompare {
	int32 operator()(ConstValue const& a, ConstValue const& b) const {
		if (a.index() == b.index()) {
			int32 result = 0;
			luisa::visit(
				[&](auto&& v) {
					using TT = decltype(v);
					using PureT = std::remove_cvref_t<TT>;
					const vstd::compare<PureT> comp;
					result = comp(v, luisa::get<PureT>(b));
				},
				a);
			return result;
		} else {
			return (a.index() > b.index()) ? 1 : -1;
		}
	}
	template<typename V>
		requires(GetVariantIndex<V, ConstValue>::valid_type)
	int32 operator()(ConstValue const& a, V const& v) {
		constexpr size_t idx = GetVariantIndex<V, ConstValue>::value;
		if (a.index() == idx) {
			return vstd::compare<V>()(luisa::get<idx>(a), v);
		} else {
			return (a.index() > idx) ? 1 : -1;
		}
	}
};
template<typename V>
using ConstValueMap = vstd::HashMap<ConstValue, V, ConstValueHash, ConstValueCompare>;
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
	constexpr bool operator==(Id id) const{
		return this->id == id.id;
	}
	constexpr bool operator!=(Id id) const{
		return this->id != id.id;
	}
	constexpr operator bool() const{
		return valid();
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
	static constexpr Id AccelUniformConstId() {
		return Id(29);
	}
	static constexpr Id AccelVarId() {
		return Id(30);
	}
	static constexpr Id SamplerId() {
		return Id(31);
	}
	static constexpr Id SamplerArrayId() {
		return Id(32);
	}
	static constexpr Id SamplerPtrId() {
		return Id(33);
	}
	static constexpr Id SamplerVarId() {
		return Id(34);
	}
	static constexpr Id SamplerArrayPtrId() {
		return Id(35);
	}
	static constexpr Id RayPayloadId(){
		return Id(36);
	}
	static constexpr Id RayPayloadPtrId(){
		return Id(37);
	}
	static constexpr Id RayPayloadVarId(){
		return Id(38);
	}
	static constexpr Id MainId() {
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