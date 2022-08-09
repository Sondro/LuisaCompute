#pragma once
#include "types.h"
namespace toolhub::spv {
class AccessLib {
	AccessLib() = delete;

public:
	static Id Load(Builder* bd, Id type, Id var);
	static void Store(Builder* bd, Id src, Id dst);
	///// Vector
	static Id Swizzle(Builder* bd, Id type, Id var, vstd::IRange<uint>& swizzles);
	static Id Swizzle(Builder* bd, Id type, Id var, std::initializer_list<uint> lst) {
		auto range = vstd::RangeImpl(vstd::CacheEndRange(lst) | vstd::ValueRange{});
		return Swizzle(bd, type, var, range);
	}
	static Id AccessVectorElement(Builder* bd, Id type, Id var, Id index);
	static Id ReadVectorElement(Builder* bd, Id type, Id value, uint index);
	///// Struct
	static Id AccessMember(Builder* bd, Id type, Id var, uint index);
	static Id ReadMember(Builder* bd, Id type, Id value, uint index);
	///// Array
	static Id AccessArrayEle(Builder* bd, Id type, Id var, Id index);
	static Id ReadArrayEle(Builder* bd, Id type, Id value, uint index);
	///// Buffer
	static Id AccessBufferEle(Builder* bd, Id type, Id var, Id bufferIndex, Id index);
	///// Matrix
	static Id AccessMatrixCol(Builder* bd, Id type, Id var, Id index);					  // return Vector<float, dimension>*
	static Id ReadMatrixCol(Builder* bd, Id type, Id value, uint index);					  // return Vector<float, dimension>*
	static Id AccessMatrixValue(Builder* bd, Id type, Id var, Id row, Id col);			  //return float*
	///// Common
	static Id AccessChain(Builder* bd, Id type, Id var, vstd::IRange<Id>& chain);
	static Id AccessChain(Builder* bd, Id type, Id var, std::initializer_list<Id> lst) {
		auto range = vstd::RangeImpl(vstd::CacheEndRange(lst) | vstd::ValueRange{});
		return AccessChain(bd, type, var, range);
	}
	static Id CompositeConstruct(Builder* bd, Id type, vstd::IRange<Id>& values);
	static Id CompositeConstruct(Builder* bd, Id type, std::initializer_list<Id> lst) {
		auto range = vstd::RangeImpl(vstd::CacheEndRange(lst) | vstd::ValueRange{});
		return CompositeConstruct(bd, type, range);
	}
};
}// namespace toolhub::spv	