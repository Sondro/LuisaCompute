#pragma once
#include "types.h"
namespace toolhub::spv {
class Variable : public Component {
public:
	Type const* type;
	Id typeId;
	Id typePtrId;
	Id varId;
	PointerUsage usage;
	Variable(Builder* bd, Type const* type, PointerUsage usage);
	///// Common
	Id Load();
	void Store(Id value);
	
	///// Vector & Struct
	Id ReadMember(uint index);
	Id Swizzle(vstd::span<uint> swizzles);
	Id AccessMember(uint index);
	void WriteMember(uint index, Id result);
	///// Array
	Id ReadArrayEle(Id index);
	Id AccessArrayEle(Id index);
	void WriteArrayEle(Id index, Id result);
	///// Buffer
	Id ReadBufferEle(Id index);
	Id AccessBufferEle(Id index);
	void WriteBufferEle(Id index, Id result);
};
}// namespace toolhub::spv