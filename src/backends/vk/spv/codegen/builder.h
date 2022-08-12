#pragma once
#include <core/mathematics.h>
#include "types.h"
#include <spirv-tools/spv_include.h>
#include "tex_descriptor.h"
#include <vstl/string_builder.h>
#include <vstl/MD5.h>
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
template<>
struct compare<luisa::compute::Type const*> {
	int32 operator()(luisa::compute::Type const* a, luisa::compute::Type const* b) const {
		return compare<size_t>()(a->hash(), b->hash());
	}
};
}// namespace vstd
namespace toolhub::spv {
class Builder : public vstd::IOperatorNewBase {
public:
	friend class Function;

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
	vstd::HashMap<vstd::MD5, Id> funcTypeMap;
	uint idCount = 0;
	Id GenStruct(Type const* type);
	Id GenConstId(ConstValue const& value);
	Id GenConstArrayId(ConstantData const& value, Id typeId);
	void AddFloat3x3Decorate(Id structId, uint memberIdx);
	TypeName& GetTypeName(Type const* type);
	TypeName& GetTypeName(TexDescriptor const& type);
	TypeName& GetTypeName(InternalType const& type);
	TypeName& GetTypeName(BufferTypeDescriptor const& type);
	vstd::vector<vstd::string> strings;
	size_t stringCount = 0;
	size_t beforeEntryHeaderIdx = 0;
	size_t afterEntryHeaderIdx = 0;
	size_t decorateStrIdx = 0;
	size_t typeStrIdx = 0;
	vstd::StringBuilder DecorateStr() { return {&strings[decorateStrIdx]}; }
	vstd::string& BeforeEntryHeader() { return strings[beforeEntryHeaderIdx]; }
	vstd::string& AfterEntryHeader() { return strings[afterEntryHeaderIdx]; }
	vstd::string* lastStr;
	bool insideFunction = false;

	Id GetTypeNamePointer(TypeName& typeName, PointerUsage usage);
	Id GetRuntimeArrayType(
		TypeName& typeName,
		PointerUsage usage,
		uint runtimeArrayStride);
	Id GetSampledImageType(
		TypeName& typeName);
	void GenBuffer(Id structId, TypeName& eleTypeName, uint arrayStride);

public:
	size_t AddString();
	vstd::StringBuilder TypeStr() { return {&strings[typeStrIdx]}; }
	vstd::StringBuilder VarStr() { return {&strings[stringCount - (insideFunction ? 2 : 1)]}; }
	Builder();
	~Builder();
	void AddKernelResource(Id resource);
	Id CBufferVar() const { return cbufferVar; }
	void GenCBuffer(vstd::IRange<luisa::compute::Variable>& args);
	vstd::StringBuilder Str() { return {lastStr}; }
	vstd::StringBuilder Str(size_t index) { return {&strings[index]}; }
	vstd::string const& Combine();
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
	Id GetConstArrayId(ConstantData const& data, Type const* type, size_t hash);
	Id ReadSampler(Id index);
	void BeginFunc();
	void EndFunc();
};
}// namespace toolhub::spv