#pragma once
#include <core/mathematics.h>
#include "types.h"
#include <spirv-tools/spv_include.h>
#include "tex_descriptor.h"
using namespace luisa;
using namespace luisa::compute;
namespace toolhub::spv {
class Variable;
class Builder {
public:
	struct TypeName {
		Id typeId;
		Id runtimeId;
		Id sampledId;
		uint runtimeArrayStride;
		Id pointerId[6];
		Id runtimePointerId[6];
	};
	using TypeDescriptor = vstd::variant<
		Type const*,
		InternalType,
		TexDescriptor>;

private:
	vstd::HashMap<TypeDescriptor, TypeName> types;
	vstd::HashMap<ConstValue, Id> constMap;
	vstd::HashMap<Id, Id> funcTypeMap;
	uint idCount = 0;
	Id GenStruct(Type const* type);
	void GenConstId(Id id, ConstValue const& value);
	void AddFloat3x3Decorate(Id structId, uint memberIdx);
	TypeName& GetTypeName(Type const* type);
	TypeName& GetTypeName(TexDescriptor const& type);
	TypeName& GetTypeName(InternalType const& type);

	Id GetTypeNamePointer(TypeName& typeName, PointerUsage usage);
	Id GetRuntimeArrayType(
		TypeName& typeName,
		PointerUsage usage,
		uint runtimeArrayStride);
	Id GetSampledImageType(
		TypeName& typeName);

public:
	bool inBlock = false;
	Id GenStruct(vstd::span<InternalType const> type);
	vstd::string header;
	vstd::string result;
	vstd::string decorateStr;
	vstd::string typeStr;
	vstd::string constValueStr;

	Id NewId() { return Id(idCount++); }
	vstd::string_view UsageName(PointerUsage usage);
	void Reset(uint3 groupSize, bool useRayTracing);
	////////////////////// Types
	Id GetTypeId(TypeDescriptor const& type, PointerUsage ptrUsage);
	Id GetRuntimeArrayTypeId(
		TypeDescriptor const& type,
		PointerUsage usage,
		uint arrayStride);
	Id GetSampledImageTypeId(
		TypeDescriptor const& type);
	std::pair<Id, Id> GetTypeAndPtrId(TypeDescriptor const& type, PointerUsage ptrUsage);
	Id GetFuncReturnTypeId(Id returnType, vstd::span<Id const> argType);
	void BindVariable(Variable const& var, uint descSet, uint binding);
	////////////////////// variable
	Id GetConstId(ConstValue const& value);
};
}// namespace toolhub::spv