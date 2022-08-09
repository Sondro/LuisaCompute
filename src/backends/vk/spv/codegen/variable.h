#pragma once
#include "types.h"
namespace toolhub::spv {
class Variable : public Component {
public:
	Type const* type;
	Id typeId;
	Id varId;
	PointerUsage usage;
	Variable(Builder* bd, Type const* type, PointerUsage usage);
	Variable(Builder* bd, Type const* type, Id varId, PointerUsage usage);
	///// Common
	Id Load() const;
	void Store(Id value) const;
	///// Vector
	Id ReadVectorElement(uint index) const;
	Id Swizzle(vstd::IRange<uint>* swizzles) const;
	Id AccessVectorElement(Id index) const;
	void WriteVectorElement(Id index, Id result) const;
	///// Struct
	Id AccessMember(uint index) const;
	Id ReadMember(uint index) const;
	void WriteMember(uint index, Id result) const;
	///// Array
	Id ReadArrayEle(Id index) const;
	Id AccessArrayEle(Id index) const;
	void WriteArrayEle(Id index, Id result) const;
	///// Buffer
	Id ReadBufferEle(Id bufferIndex, Id index) const;
	Id AccessBufferEle(Id bufferIndex, Id index) const;
	void WriteBufferEle(Id bufferIndex, Id index, Id result) const;
	///// Matrix
	Id AccessMatrixCol(Id index) const;					 // return Vector<float, dimension>*
	Id AccessMatrixValue(Id row, Id col) const;			 //return float*
	Id ReadMatrixCol(Id index) const;						 //return Vector<float, dimension>
	Id ReadMatrixValue(Id row, Id col) const;				 //return float
	void WriteMatrixCol(Id index, Id result) const;		 // write Vector<float, dimension>
	void WriteMatrixValue(Id row, Id col, Id result) const;//write float
	Id AccessChain(Id typeId, vstd::IRange<Id>& chain);
	Id AccessChain(Id typeId, std::initializer_list<Id> lst){
		auto range = vstd::RangeImpl(vstd::CacheEndRange(lst) | vstd::ValueRange{});
		return AccessChain(typeId, range);
	}
};
}// namespace toolhub::spv