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
	Id Load();
	void Store(Id value);
	///// Vector
	Id ReadVectorElement(uint index);
	Id Swizzle(vstd::IRange<uint>* swizzles);
	Id AccessVectorElement(uint index);
	void WriteVectorElement(uint index, Id result);
	///// Struct
	Id AccessMember(uint index);
	Id ReadMember(uint index);
	void WriteMember(uint index, Id result);
	///// Array
	Id ReadArrayEle(Id index);
	Id AccessArrayEle(Id index);
	void WriteArrayEle(Id index, Id result);
	///// Buffer
	Id ReadBufferEle(Id bufferIndex, Id index);
	Id AccessBufferEle(Id bufferIndex, Id index);
	void WriteBufferEle(Id bufferIndex, Id index, Id result);
	///// Matrix
	Id AccessMatrixCol(Id index);					 // return Vector<float, dimension>*
	Id AccessMatrixValue(Id row, Id col);			 //return float*
	Id ReadMatrixCol(Id index);						 //return Vector<float, dimension>
	Id ReadMatrixValue(Id row, Id col);				 //return float
	void WriteMatrixCol(Id index, Id result);		 // write Vector<float, dimension>
	void WriteMatrixValue(Id row, Id col, Id result);//write float
};
}// namespace toolhub::spv