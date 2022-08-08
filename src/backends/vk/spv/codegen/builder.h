#pragma once
#include <core/mathematics.h>
#include "types.h"
#include <spirv-tools/spv_include.h>
#include "tex_descriptor.h"
#include <vstl/string_builder.h>
using namespace luisa;
using namespace luisa::compute;
namespace toolhub::spv {
class Function;
class Variable;
struct BufferTypeDescriptor {
	InternalType type;
	explicit BufferTypeDescriptor(InternalType type) : type(type) {}
};
}// namespace toolhub::spv
namespace vstd {
template<>
struct hash<luisa::compute::Type const*> {
	size_t operator()(luisa::compute::Type const* t) const {
		return t->hash();
	}
};
}// namespace vstd
namespace toolhub::spv {
class Builder : public vstd::IOperatorNewBase {
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
		TexDescriptor,
		BufferTypeDescriptor>;

private:
	vstd::optional<TypeName> cbufferType;
	vstd::optional<TypeName> kernelArgType;
	Id bdlsTex2D;
	Id bdlsTex3D;
	//Id bdlsBuffer;
	size_t cbufferSize;
	Id cbufferVar;
	vstd::HashMap<TypeDescriptor, TypeName> types;
	ConstValueMap<Id> constMap;
	vstd::HashMap<uint64, Id> constArrMap;
	vstd::HashMap<Id, Id> funcTypeMap;
	uint idCount = 0;
	Id GenStruct(Type const* type);
	Id GenConstId(ConstValue const& value);
	Id GenConstArrayId(ConstantData const& value, Id typeId);
	void AddFloat3x3Decorate(Id structId, uint memberIdx);
	TypeName& GetTypeName(Type const* type);
	TypeName& GetTypeName(TexDescriptor const& type);
	TypeName& GetTypeName(InternalType const& type);
	TypeName& GetTypeName(BufferTypeDescriptor const& type);

	Id GetTypeNamePointer(TypeName& typeName, PointerUsage usage);
	Id GetRuntimeArrayType(
		TypeName& typeName,
		PointerUsage usage,
		uint runtimeArrayStride);
	Id GetSampledImageType(
		TypeName& typeName);
	vstd::string bodyStr;
	vstd::string beforeEntryHeader;
	vstd::string entryStr;
	vstd::string afterEntryHeader;
	vstd::string decorateStr;
	vstd::string typeStr;
	vstd::string constValueStr;
	void GenBuffer(Id structId, TypeName& eleTypeName, uint arrayStride);

public:
	void AddKernelResource(Id resource);
	Id CBufferVar() const { return cbufferVar; }
	void GenCBuffer(vstd::IRange<luisa::compute::Variable>& args);
	vstd::StringBuilder TypeStr() { return {&typeStr}; }
	vstd::StringBuilder Str() { return {&bodyStr}; }
	vstd::string Combine();
	//void GenConstBuffer();
	bool inBlock = false;
	std::pair<Id, size_t> GenStruct(
		vstd::IRange<
			vstd::variant<
				Type const*,
				InternalType>>& type);

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
		TexDescriptor const& type);
	std::pair<Id, Id> GetTypeAndPtrId(TypeDescriptor const& type, PointerUsage ptrUsage);
	Id GetFuncReturnTypeId(Id returnType, vstd::IRange<Id>* argType);
	void BindVariable(Id varId, uint descSet, uint binding);
	void BindCBuffer(uint binding);
	////////////////////// variable
	Id GetConstId(ConstValue const& value);
	Id GetConstArrayId(ConstantData const& data, Type const* type);
	Id ReadSampler(Id index);
};
}// namespace toolhub::spv