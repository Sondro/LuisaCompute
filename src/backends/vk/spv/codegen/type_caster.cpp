#include "type_caster.h"
#include "builder.h"
namespace toolhub::spv {
TypeCaster::CompareResult TypeCaster::Compare(
	InternalType const& srcType,
	InternalType const& dstType) {
	auto Weight = [](InternalType const& t) {
		uint scalarWeights[4] = {
			0, 3, 2, 1};
		switch (t.tag) {
			case InternalType::Tag::BOOL:
			case InternalType::Tag::FLOAT:
			case InternalType::Tag::INT:
			case InternalType::Tag::UINT:
				return scalarWeights[static_cast<uint>(t.tag)] + t.dimension * 10;
			default:
				return t.dimension * t.dimension * 10000;
		}
	};
	auto leftWeight = Weight(srcType);
	auto rightWeight = Weight(dstType);
	if (leftWeight > rightWeight) return CompareResult::ToLeft;
	if (leftWeight == rightWeight) return CompareResult::UnChanged;
	return CompareResult::ToRight;
}
namespace detail {
template<typename T>
static Id ConstructVector(Builder* bd, uint dimension, int32 value) {
	switch (dimension) {
		case 2:
			return bd->GetConstId(Vector<T, 2>(value, value));
		case 3:
			return bd->GetConstId(Vector<T, 3>(value, value, value));
		case 4:
			return bd->GetConstId(Vector<T, 4>(value, value, value, value));
		default: return Id();
	}
}
static Id ConstructMatrix(Builder* bd, uint dimension, int32 value) {
	switch (dimension) {
		case 2:
			return bd->GetConstId(Matrix<2>(float2(value, value), float2(value, value)));
		case 3:
			return bd->GetConstId(Matrix<3>(
				float3(value, value, value),
				float3(value, value, value),
				float3(value, value, value)));
		case 4:
			return bd->GetConstId(Matrix<4>(
				float4(value, value, value, value),
				float4(value, value, value, value),
				float4(value, value, value, value),
				float4(value, value, value, value)));
		default: return Id();
	}
}
}// namespace detail
Id TypeCaster::Cast(
	Builder* bd,
	InternalType const& srcType,
	InternalType const& dstType,
	Id value) {
	Id newId(bd->NewId());
	bd->Str() << newId.ToString();
	auto GetConst = [&](InternalType const& tp, int32 value) {
		if (tp.dimension <= 1) {
			switch (tp.tag) {
				case InternalType::Tag::BOOL:
					return value == 0 ? Id::FalseId() : Id::TrueId();
				case InternalType::Tag::MATRIX:
				case InternalType::Tag::FLOAT:
					return bd->GetConstId((float)value);
				case InternalType::Tag::INT:
					return bd->GetConstId(value);
				case InternalType::Tag::UINT:
					return bd->GetConstId((uint)value);
			}
		} else {
			switch (tp.tag) {
				case InternalType::Tag::BOOL:
					return detail::ConstructVector<bool>(bd, tp.dimension, value);
				case InternalType::Tag::FLOAT:
					return detail::ConstructVector<float>(bd, tp.dimension, value);
				case InternalType::Tag::INT:
					return detail::ConstructVector<int>(bd, tp.dimension, value);
				case InternalType::Tag::UINT:
					return detail::ConstructVector<uint>(bd, tp.dimension, value);
				case InternalType::Tag::MATRIX:
					return detail::ConstructMatrix(bd, tp.dimension, value);
			}
		};
	};
	if (srcType.tag == InternalType::Tag::BOOL) {
		auto constZero = GetConst(dstType, 0);
		auto constOne = GetConst(dstType, 1);
		bd->Str() << " = OpSelect "sv << dstType.TypeId().ToString() << ' ' << value.ToString() << ' ' << constZero.ToString() << ' ' << constOne.ToString() << '\n';
	} else if (dstType.tag == InternalType::Tag::BOOL) {
		auto constZero = GetConst(dstType, 0);
		bd->Str() << " = OpINotEqual "sv << dstType.TypeId().ToString() << ' ' << value.ToString() << ' ' << constZero.ToString() << '\n';
	} else {
		switch (srcType.tag) {
			case InternalType::Tag::FLOAT:
			case InternalType::Tag::MATRIX:
				switch (dstType.tag) {
					case InternalType::Tag::FLOAT:
					case InternalType::Tag::MATRIX:
						bd->Str() << " = OpBitcast "sv << dstType.TypeId().ToString() << ' ' << value.ToString() << '\n';
						break;
					case InternalType::Tag::INT:
						bd->Str() << " = OpConvertFToS "sv << dstType.TypeId().ToString() << ' ' << value.ToString() << '\n';
						break;
					case InternalType::Tag::UINT:
						bd->Str() << " = OpConvertFToU "sv << dstType.TypeId().ToString() << ' ' << value.ToString() << '\n';
						break;
				}
				break;
			case InternalType::Tag::INT:
				switch (dstType.tag) {
					case InternalType::Tag::FLOAT:
					case InternalType::Tag::MATRIX:
						bd->Str() << " = OpConvertSToF "sv << dstType.TypeId().ToString() << ' ' << value.ToString() << '\n';
						break;
					case InternalType::Tag::UINT:
					case InternalType::Tag::INT:
						bd->Str() << " = OpBitcast "sv << dstType.TypeId().ToString() << ' ' << value.ToString() << '\n';
						break;
				}
				break;
			case InternalType::Tag::UINT:
				switch (dstType.tag) {
					case InternalType::Tag::FLOAT:
					case InternalType::Tag::MATRIX:
						bd->Str() << " = OpConvertUToF "sv << dstType.TypeId().ToString() << ' ' << value.ToString() << '\n';
						break;
					case InternalType::Tag::UINT:
					case InternalType::Tag::INT:
						bd->Str() << " = OpBitcast "sv << dstType.TypeId().ToString() << ' ' << value.ToString() << '\n';
						break;
				}
				break;
		}
		bd->Str() << dstType.TypeId().ToString() << ' ' << value.ToString() << '\n';
	}
	return newId;
}
}// namespace toolhub::spv