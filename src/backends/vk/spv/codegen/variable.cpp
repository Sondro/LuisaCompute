#include "variable.h"
#include "builder.h"
namespace toolhub::spv {
Variable::Variable(Builder* bd, Type const* type, PointerUsage usage)
	: Component(bd), type(type), usage(usage) {
	auto typeAndPtrId = bd->GetTypeAndPtrId(type, usage);
	typePtrId = typeAndPtrId.second;
	typeId = typeAndPtrId.first;
	varId = bd->NewId();
	bd->result << varId.ToString() << " = OpVariable "_sv << typePtrId.ToString() << ' ' << bd->UsageName(usage) << '\n';
}
Id Variable::Swizzle(vstd::span<uint> swizzles) {
	Id newId(bd->NewId());
	bd->result << newId.ToString() << " = OpVectorShuffle "_sv << typeId.ToString() << ' ' << typeId.ToString() << ' ';
	for (auto& i : swizzles) {
		bd->result << vstd::to_string(i) << ' ';
	}
	bd->result << '\n';
	return newId;
}
Id Variable::ReadMember(uint index) {
	auto loadId = AccessMember(index);
	Id newId(bd->NewId());
	bd->result << newId.ToString() << " = OpLoad "_sv << bd->GetTypeId(type->element(), PointerUsage::NotPointer).ToString() << ' ' << loadId.ToString() << '\n';
	return newId;
}
Id Variable::Load() {
	Id newId(bd->NewId());
	bd->result << newId.ToString() << " = OpLoad "_sv << typeId.ToString() << ' ' << varId.ToString() << '\n';
	return newId;
}

void Variable::Store(Id value) {
	bd->result << "OpStore "_sv << varId.ToString() << ' ' << value.ToString() << '\n';
}
Id Variable::AccessMember(uint index) {
	Id acsChain(bd->NewId());
	bd->result << acsChain.ToString() << " = OpAccessChain "_sv << bd->GetTypeId(type->element(), usage).ToString() << ' ' << varId.ToString() << ' ' << bd->GetConstId(index).ToString() << '\n';
	return acsChain;
}

void Variable::WriteMember(uint index, Id result) {
	auto chain = AccessMember(index);
	bd->result << "OpStore "_sv << chain.ToString() << ' ' << result.ToString() << '\n';
}
Id Variable::AccessArrayEle(Id index) {
	Id acsChain(bd->NewId());
	bd->result << acsChain.ToString() << " = OpAccessChain "_sv << bd->GetTypeId(type->element(), usage).ToString() << ' ' << varId.ToString() << ' ' << index.ToString() << '\n';
	return acsChain;
}
Id Variable::ReadArrayEle(Id index) {
	auto loadId = AccessArrayEle(index);
	Id newId(bd->NewId());
	bd->result << newId.ToString() << " = OpLoad "_sv << bd->GetTypeId(type->element(), PointerUsage::NotPointer).ToString() << ' ' << loadId.ToString() << '\n';
	return newId;
}
void Variable::WriteArrayEle(Id index, Id result) {
	auto chain = AccessArrayEle(index);
	bd->result << "OpStore "_sv << chain.ToString() << ' ' << result.ToString() << '\n';
}
Id Variable::AccessBufferEle(Id index) {
	Id acsChain(bd->NewId());
	bd->result << acsChain.ToString() << " = OpAccessChain "_sv << bd->GetTypeId(type->element(), usage).ToString() << ' ' << Id::ZeroId().ToString() << ' ' << varId.ToString() << ' ' << index.ToString() << '\n';
	return acsChain;
}
Id Variable::ReadBufferEle(Id index) {
	auto loadId = AccessBufferEle(index);
	Id newId(bd->NewId());
	bd->result << newId.ToString() << " = OpLoad "_sv << bd->GetTypeId(type->element(), PointerUsage::NotPointer).ToString() << ' ' << loadId.ToString() << '\n';
	return newId;
}
void Variable::WriteBufferEle(Id index, Id result) {
	auto chain = AccessBufferEle(index);
	bd->result << "OpStore "_sv << chain.ToString() << ' ' << result.ToString() << '\n';
}
}// namespace toolhub::spv