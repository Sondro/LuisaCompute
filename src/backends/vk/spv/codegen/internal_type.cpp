#include "internal_type.h"
namespace toolhub::spv {
vstd::optional<InternalType> InternalType::GetType(Type const* type) {
	if (static_cast<uint>(type->tag()) > static_cast<uint>(Type::Tag::MATRIX)) return {};
	vstd::optional<InternalType> tp;
	tp.New();
	switch (type->tag()) {
		case Type::Tag::MATRIX:
			tp->tag = Tag::MATRIX;
			tp->dimension = type->dimension();
			break;
		case Type::Tag::VECTOR:
			tp->dimension = type->dimension();
			tp->tag = static_cast<Tag>(type->element()->tag());
			break;
		default:
			tp->tag = static_cast<Tag>(type->tag());
			tp->dimension = 1;
			break;
	}
	if (type->element()) {
		tp->tag = static_cast<Tag>(type->element()->tag());
	}
	return tp;
}
Id InternalType::TypeId() const {
	if (tag == Tag::MATRIX) {
		return Id::MatId(dimension);
	} else {
		auto scalarTag = [&] {
			switch (tag) {
				case Tag::BOOL:
					return Id::BoolId();
				case Tag::FLOAT:
					return Id::FloatId();
				case Tag::INT:
					return Id::IntId();
				case Tag::UINT:
					return Id::UIntId();
			}
		}();
		if (dimension > 1) {
			scalarTag = Id::VecId(scalarTag, dimension);
		}
		return scalarTag;
	}
}
size_t InternalType::Align() const {
	auto dimByte = [](size_t byte, size_t dim) {
		if (dim == 3) dim = 4;
		return byte * dim;
	};
	return dimByte(
		[&] {
			switch (tag) {
				case InternalType::Tag::BOOL:
					return 1;
				default: return 4;
			}
		}(),
		dimension);
}
size_t InternalType::Size() const {
	auto dimByte = [](size_t byte, size_t dim) {
		if (dim == 3) dim = 4;
		return byte * dim;
	};
	return dimByte(
		[&] -> uint {
			switch (tag) {
				case InternalType::Tag::BOOL:
					return 1;
				case InternalType::Tag::FLOAT:
				case InternalType::Tag::UINT:
				case InternalType::Tag::INT:
					return 4;
				case InternalType::Tag::MATRIX:
					return 4 * dimension;
				default: return 4;
			}
		}(),
		dimension);
}
ConstValue InternalType::LiteralValue(vstd::variant<bool, int32, uint, float> var) {
	auto GetValue = [&]<Tag tag> {
		if constexpr (tag == Tag::BOOL) {
			return var.visit_or(
				false,
				[](auto v) {
					if constexpr (std::is_same_v<bool, decltype(v)>) {
						return v;
					} else {
						return v != 0;
					}
				});
		} else if constexpr (tag == Tag::UINT) {
			return var.visit_or(
				0u,
				[](auto v) { return static_cast<uint>(v); });
		} else if constexpr (tag == Tag::INT) {
			return var.visit_or(
				0,
				[](auto v) { return static_cast<int32>(v); });
		} else {
			return var.visit_or(
				0.0f,
				[](auto v) { return static_cast<float>(v); });
		}
	};
	switch (tag) {
		case Tag::BOOL: {
			auto value = GetValue.operator()<Tag::BOOL>();
			switch (dimension) {
				case 1:
					return value;
				case 2:
					return bool2(value, value);
				case 3:
					return bool3(value, value, value);
				case 4:
					return bool4(value, value, value, value);
				default:
					assert(false);
			}
		}
		case Tag::UINT: {
			auto value = GetValue.operator()<Tag::UINT>();
			switch (dimension) {
				case 1:
					return value;
				case 2:
					return uint2(value, value);
				case 3:
					return uint3(value, value, value);
				case 4:
					return uint4(value, value, value, value);
				default:
					assert(false);
			}
		}
		case Tag::INT: {
			auto value = GetValue.operator()<Tag::INT>();
			switch (dimension) {
				case 1:
					return value;
				case 2:
					return int2(value, value);
				case 3:
					return int3(value, value, value);
				case 4:
					return int4(value, value, value, value);
				default:
					assert(false);
			}
		}
		case Tag::FLOAT: {
			auto value = GetValue.operator()<Tag::FLOAT>();
			switch (dimension) {
				case 1:
					return value;
				case 2:
					return float2(value, value);
				case 3:
					return float3(value, value, value);
				case 4:
					return float4(value, value, value, value);
				default:
					assert(false);
			}
		}
		case Tag::MATRIX: {
			auto value = GetValue.operator()<Tag::MATRIX>();
			switch (dimension) {
				case 2:
					return float2x2(
						float2(value, value),
						float2(value, value));
				case 3:
					return float3x3(
						float3(value, value, value),
						float3(value, value, value),
						float3(value, value, value));
				case 4:
					return float4x4(
						float4(value, value, value, value),
						float4(value, value, value, value),
						float4(value, value, value, value),
						float4(value, value, value, value));
				default:
					assert(false);
			}
		}
		default:
			assert(false);
	}
}
}// namespace toolhub::spv