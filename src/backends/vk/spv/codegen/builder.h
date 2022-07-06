#pragma once
#include <core/mathematics.h>
#include "types.h"
#include <spirv-tools/spv_include.h>
using namespace luisa;
using namespace luisa::compute;
namespace toolhub::spv {
class Variable;
class Builder {
public:
	struct TypeName {
		Id typeId;
		Id pointerId[6];
	};

private:
	vstd::HashMap<Type const*, TypeName> types;
	vstd::HashMap<ConstValue, Id> constMap;
	vstd::HashMap<Id, Id> funcTypeMap;
	uint idCount = 0;
	Id GenStruct(Type const* type);
	void GenConstId(Id id, ConstValue const& value);
	TypeName& GetTypeName(Type const* type);

public:
	vstd::string result;
	vstd::string decorateStr;
	vstd::string typeStr;
	vstd::string constValueStr;

	Id NewId() { return Id(idCount++); }
	vstd::string_view UsageName(PointerUsage usage);
	void Reset(uint3 groupSize);
	////////////////////// Types
	Id GetTypeId(Type const* type, PointerUsage ptrUsage);
	std::pair<Id, Id> GetTypeAndPtrId(Type const* type, PointerUsage ptrUsage);
	Id GetFuncReturnTypeId(Id returnType);
	void BindVariable(Variable const& var, uint descSet, uint binding);
	////////////////////// variable
	Id GetConstId(ConstValue const& value);
	//Variable GetVariable();
};
}// namespace toolhub::spv