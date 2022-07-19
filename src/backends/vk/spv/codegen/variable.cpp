#include "variable.h"
#include "builder.h"
namespace toolhub::spv {
Variable::Variable(Builder* bd, Type const* type, PointerUsage usage)
	: Component(bd), type(type), usage(usage) {
	auto typeAndPtrId = bd->GetTypeAndPtrId(type, usage);
	auto typePtrId = typeAndPtrId.second;
	typeId = typeAndPtrId.first;
	varId = bd->NewId();
	bd->Str() << varId.ToString() << " = OpVariable "sv << typePtrId.ToString() << ' ' << bd->UsageName(usage) << '\n';
}
Variable::Variable(Builder* bd, Type const* type, Id varId, PointerUsage usage)
	: Component(bd), type(type), usage(usage), varId(varId) {
	typeId = bd->GetTypeId(type, usage);
}

Id Variable::Swizzle(vstd::IRange<uint>* swizzles) {
	Id newId(bd->NewId());
	bd->Str() << newId.ToString() << " = OpVectorShuffle "sv << typeId.ToString() << ' ' << typeId.ToString() << ' ';
	for (auto i : *swizzles) {
		bd->Str() << vstd::to_string(i) << ' ';
	}
	bd->Str() << '\n';
	return newId;
}
Id Variable::ReadVectorElement(uint index) {
	auto loadId = AccessVectorElement(index);
	Id newId(bd->NewId());
	bd->Str() << newId.ToString() << " = OpLoad "sv << bd->GetTypeId(type->element(), PointerUsage::NotPointer).ToString() << ' ' << loadId.ToString() << '\n';
	return newId;
}
Id Variable::AccessMember(uint index) {
	Id acsChain(bd->NewId());
	bd->Str() << acsChain.ToString() << " = OpAccessChain "sv << bd->GetTypeId(type->members()[index], usage).ToString() << ' ' << varId.ToString() << ' ' << bd->GetConstId(index).ToString() << '\n';
	return acsChain;
}
Id Variable::ReadMember(uint index) {
	auto loadId = AccessMember(index);
	Id newId(bd->NewId());
	bd->Str() << newId.ToString() << " = OpLoad "sv << bd->GetTypeId(type->members()[index], PointerUsage::NotPointer).ToString() << ' ' << loadId.ToString() << '\n';
	return newId;
}
void Variable::WriteMember(uint index, Id result) {
	auto chain = AccessMember(index);
	bd->Str() << "OpStore "sv << chain.ToString() << ' ' << result.ToString() << '\n';
}
Id Variable::Load() {
	Id newId(bd->NewId());
	bd->Str() << newId.ToString() << " = OpLoad "sv << typeId.ToString() << ' ' << varId.ToString() << '\n';
	return newId;
}

void Variable::Store(Id value) {
	bd->Str() << "OpStore "sv << varId.ToString() << ' ' << value.ToString() << '\n';
}
Id Variable::AccessVectorElement(uint index) {
	Id acsChain(bd->NewId());
	bd->Str() << acsChain.ToString() << " = OpAccessChain "sv << bd->GetTypeId(type->element(), usage).ToString() << ' ' << varId.ToString() << ' ' << bd->GetConstId(index).ToString() << '\n';
	return acsChain;
}

void Variable::WriteVectorElement(uint index, Id result) {
	auto chain = AccessVectorElement(index);
	bd->Str() << "OpStore "sv << chain.ToString() << ' ' << result.ToString() << '\n';
}
Id Variable::AccessArrayEle(Id index) {
	Id acsChain(bd->NewId());
	bd->Str() << acsChain.ToString() << " = OpAccessChain "sv << bd->GetTypeId(type->element(), usage).ToString() << ' ' << varId.ToString() << ' ' << index.ToString() << '\n';
	return acsChain;
}
Id Variable::ReadArrayEle(Id index) {
	auto loadId = AccessArrayEle(index);
	Id newId(bd->NewId());
	bd->Str() << newId.ToString() << " = OpLoad "sv << bd->GetTypeId(type->element(), PointerUsage::NotPointer).ToString() << ' ' << loadId.ToString() << '\n';
	return newId;
}
void Variable::WriteArrayEle(Id index, Id result) {
	auto chain = AccessArrayEle(index);
	bd->Str() << "OpStore "sv << chain.ToString() << ' ' << result.ToString() << '\n';
}
Id Variable::AccessBufferEle(Id bufferIndex, Id index) {
	Id acsChain(bd->NewId());
	bd->Str() << acsChain.ToString() << " = OpAccessChain "sv << bd->GetTypeId(type->element(), usage).ToString() << ' ' << varId.ToString() << ' ' << bufferIndex.ToString() << ' ' << index.ToString() << '\n';
	return acsChain;
}
Id Variable::ReadBufferEle(Id bufferIndex, Id index) {
	auto loadId = AccessBufferEle(bufferIndex, index);
	Id newId(bd->NewId());
	bd->Str() << newId.ToString() << " = OpLoad "sv << bd->GetTypeId(type->element(), PointerUsage::NotPointer).ToString() << ' ' << loadId.ToString() << '\n';
	return newId;
}
void Variable::WriteBufferEle(Id bufferIndex, Id index, Id result) {
	auto chain = AccessBufferEle(bufferIndex, index);
	bd->Str() << "OpStore "sv << chain.ToString() << ' ' << result.ToString() << '\n';
}
Id Variable::AccessMatrixCol(Id index) {
	Id newId(bd->NewId());
	bd->Str() << newId.ToString() << "OpAccessChain "sv << bd->GetTypeId(InternalType(InternalType::Tag::FLOAT, type->dimension()), usage).ToString() << ' ' << varId.ToString() << ' ' << index.ToString() << '\n';
	return newId;
}
Id Variable::AccessMatrixValue(Id row, Id col) {
	Id newId(bd->NewId());
	bd->Str() << newId.ToString() << "OpAccessChain "sv << bd->GetTypeId(InternalType(InternalType::Tag::FLOAT, type->dimension()), usage).ToString() << ' ' << varId.ToString() << ' ' << row.ToString() << ' ' << col.ToString() << '\n';
	return newId;
}
Id Variable::ReadMatrixCol(Id index) {
	auto acs = AccessMatrixCol(index);
	Id read(bd->NewId());
	bd->Str() << read.ToString() << " = OpLoad "sv << bd->GetTypeId(InternalType(InternalType::Tag::FLOAT, type->dimension()), PointerUsage::NotPointer).ToString() << ' ' << read.ToString() << '\n';
	return read;
}
Id Variable::ReadMatrixValue(Id row, Id col) {
	auto acs = AccessMatrixValue(row, col);
	Id read(bd->NewId());
	bd->Str() << read.ToString() << " = OpLoad "sv << bd->GetTypeId(InternalType(InternalType::Tag::FLOAT, type->dimension()), PointerUsage::NotPointer).ToString() << ' ' << read.ToString() << '\n';
	return read;
}
void Variable::WriteMatrixCol(Id index, Id result) {
	auto acs = AccessMatrixCol(index);
	bd->Str() << "OpStore "sv << acs.ToString() << ' ' << result.ToString() << '\n';
}
void Variable::WriteMatrixValue(Id row, Id col, Id result) {
	auto acs = AccessMatrixValue(row, col);
	bd->Str() << "OpStore "sv << acs.ToString() << ' ' << result.ToString() << '\n';
}
}// namespace toolhub::spv