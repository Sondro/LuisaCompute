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
				return scalarWeights[static_cast<uint>(t.tag)];
		}
	};
	auto leftWeight = Weight(srcType);
	auto rightWeight = Weight(dstType);
	if (leftWeight > rightWeight) return CompareResult::ToLeft;
	if (leftWeight == rightWeight) return CompareResult::UnChanged;
	return CompareResult::ToRight;
}
Id TypeCaster::Cast(
	Builder* bd,
	InternalType const& srcType,
	InternalType const& dstType,
	Id value) {
	Id newId(bd->NewId());
	bd->result << newId.ToString();
	if (srcType.tag == InternalType::Tag::BOOL) {
//OpSelect %dst_type %0_const %1_const
	} else if (dstType.tag == InternalType::Tag::BOOL) {
// OpINotEqual
	} else {
		switch (srcType.tag) {
			case InternalType::Tag::FLOAT:
			case InternalType::Tag::MATRIX:
				bd->result << " = OpConvertF"_sv;
				break;
			case InternalType::Tag::INT:
				bd->result << " = OpConvertS"_sv;
				break;
			case InternalType::Tag::UINT:
				bd->result << " = OpConvertU"_sv;
				break;
		}
		switch (dstType.tag) {
			case InternalType::Tag::FLOAT:
			case InternalType::Tag::MATRIX:
				bd->result << "ToF "_sv;
				break;
			case InternalType::Tag::INT:
				bd->result << "ToS "_sv;
				break;
			case InternalType::Tag::UINT:
				bd->result << "ToU "_sv;
				break;
		}
	}
	bd->result << dstType.TypeId().ToString() << ' ' << value.ToString() << '\n';
	return newId;
}
}// namespace toolhub::spv