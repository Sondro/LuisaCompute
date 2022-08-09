#include "access_lib.h"
#include "builder.h"
namespace toolhub::spv {
Id AccessLib::Load(Builder* bd, Id type, Id var) {
	Id newId{bd->NewId()};
	bd->Str() << newId.ToString() << " = OpLoad "sv << type.ToString() << ' ' << var.ToString() << '\n';
	return newId;
}
void AccessLib::Store(Builder* bd, Id src, Id dst) {
	bd->Str() << "OpStore "sv << dst.ToString() << ' ' << src.ToString() << '\n';
}
///// Vector
Id AccessLib::Swizzle(Builder* bd, Id type, Id var, vstd::IRange<uint>& swizzles) {
	Id newId{bd->NewId()};
	auto str = Load(bd, type, var).ToString();
	bd->Str() << newId.ToString() << " = OpVectorShuffle "sv << type.ToString() << ' ' << vstd::string_view(str) << ' ' << vstd::string_view(str);
	for (auto i : swizzles) {
		bd->Str() << ' ' << vstd::to_string(i);
	}
	bd->Str() << '\n';
	return newId;
}
Id AccessLib::AccessVectorElement(Builder* bd, Id type, Id var, Id index) {
	return AccessArrayEle(bd, type, var, index);
}
Id AccessLib::ReadMember(Builder* bd, Id type, Id var, uint index) {
	Id newId{bd->NewId()};
	bd->Str() << newId.ToString() << " = OpCompositeExtract "sv << type.ToString() << ' ' << var.ToString() << ' ' << vstd::to_string(index) << '\n';
	return newId;
}
Id AccessLib::ReadVectorElement(Builder* bd, Id type, Id var, uint index) {
	return ReadMember(bd, type, var, index);
}
Id AccessLib::ReadArrayEle(Builder* bd, Id type, Id var, uint index) {
	return ReadMember(bd, type, var, index);
}
Id AccessLib::ReadMatrixCol(Builder* bd, Id type, Id var, uint index) {
	return ReadMember(bd, type, var, index);
}
///// Struct
Id AccessLib::AccessMember(Builder* bd, Id type, Id var, uint index) {
	return AccessArrayEle(bd, type, var, bd->GetConstId(index));
}
///// Array
Id AccessLib::AccessArrayEle(Builder* bd, Id type, Id var, Id index) {
	Id acsChain{bd->NewId()};
	bd->Str() << acsChain.ToString() << " = OpAccessChain "sv << type.ToString() << ' ' << var.ToString() << ' ' << index.ToString() << '\n';
	return acsChain;
}
///// Buffer
Id AccessLib::AccessBufferEle(Builder* bd, Id type, Id var, Id bufferIndex, Id index) {
	Id acsChain{bd->NewId()};
	bd->Str() << acsChain.ToString() << " = OpAccessChain "sv << type.ToString() << ' ' << var.ToString() << ' ' << ' ' << bufferIndex.ToString() << index.ToString() << '\n';
	return acsChain;
}
///// Matrix
Id AccessLib::AccessMatrixCol(Builder* bd, Id type, Id var, Id index) {
	return AccessArrayEle(bd, type, var, index);
}// return Vector<float, dimension>*
Id AccessLib::AccessMatrixValue(Builder* bd, Id type, Id var, Id row, Id col) {
	Id acsChain{bd->NewId()};
	bd->Str() << acsChain.ToString() << " = OpAccessChain "sv << type.ToString() << ' ' << var.ToString() << ' ' << col.ToString() << ' ' << row.ToString() << '\n';
	return acsChain;
}//return float*
///// Common
Id AccessLib::AccessChain(Builder* bd, Id type, Id var, vstd::IRange<Id>& chain) {
	Id acsChain{bd->NewId()};
	auto builder = bd->Str();
	builder << acsChain.ToString() << " = OpAccessChain "sv << type.ToString() << ' ' << var.ToString();
	for (auto&& i : chain) {
		builder << ' ' << i.ToString();
	}
	builder << '\n';
	return acsChain;
}
Id AccessLib::CompositeConstruct(Builder* bd, Id type, vstd::IRange<Id>& values) {
	Id newId{bd->NewId()};
	auto builder = bd->Str();
	builder << newId.ToString() << " = OpCompositeConstruct "sv << type.ToString();
	for (auto&& i : values) {
		builder << ' ' << i.ToString();
	}
	builder << '\n';
	return newId;
}
}// namespace toolhub::spv