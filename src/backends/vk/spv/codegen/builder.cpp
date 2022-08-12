#include "builder.h"
#include <vulkan_include.h>
#include "variable.h"
namespace toolhub::spv {
Builder::Builder() {
	strings.reserve(64);
}
Builder::~Builder() {}
namespace Builder_detail {
// float: 0
// int: 1
// uint: 2
// bool: 3
// vec: 4 + 3 * ele_type + (dim - 2)
// mat: 16 + dim - 2
// void: 19
static void AddInternalTypeCode(vstd::StringBuilder&& str) {
	str << R"(
%0 = OpTypeFloat 32
%1 = OpTypeInt 32 1
%2 = OpTypeInt 32 0
%3 = OpTypeBool
%4 = OpTypeVector %0 2
%5 = OpTypeVector %0 3
%6 = OpTypeVector %0 4
%7 = OpTypeVector %1 2
%8 = OpTypeVector %1 3
%9 = OpTypeVector %1 4
%10 = OpTypeVector %2 2
%11 = OpTypeVector %2 3
%12 = OpTypeVector %2 4
%13 = OpTypeVector %3 2
%14 = OpTypeVector %3 3
%15 = OpTypeVector %3 4
%16 = OpTypeMatrix %4 2
%17 = OpTypeMatrix %5 3
%18 = OpTypeMatrix %6 4
%19 = OpConstantTrue %3
%20 = OpConstantFalse %3
%21 = OpConstant %2 0
%22 = OpTypeVoid
%31 = OpTypeSampler
)"sv;
}
static vstd::string_view TexFormatName(InternalType const& texDesc) {
	switch (texDesc.tag) {
		case InternalType::Tag::FLOAT:
			if (texDesc.dimension <= 1)
				return "R32f"sv;
			else
				return "Rgba32f"sv;
		case InternalType::Tag::UINT:
		case InternalType::Tag::INT:
			if (texDesc.dimension <= 1)
				return "R32ui"sv;
			else
				return "Rgba32ui"sv;
			break;
		default: return ""sv;
	}
}
template<typename T>
struct VecInfo {
	static constexpr bool isVector = false;
	static constexpr bool isMatrix = false;
};
template<typename Ele, size_t dim>
struct VecInfo<Vector<Ele, dim>> {
	static constexpr bool isVector = true;
	static constexpr bool isMatrix = false;
	using elementType = Ele;
	static constexpr size_t dimension = dim;
	static constexpr Id VecId() {
		if constexpr (std::is_same_v<Ele, float>) {
			return Id::VecId(Id::FloatId(), dimension);
		}
		if constexpr (std::is_same_v<Ele, int32>) {
			return Id::VecId(Id::IntId(), dimension);
		}
		if constexpr (std::is_same_v<Ele, uint>) {
			return Id::VecId(Id::UIntId(), dimension);
		}
		if constexpr (std::is_same_v<Ele, bool>) {
			return Id::VecId(Id::BoolId(), dimension);
		}
	}
};
template<size_t dim>
struct VecInfo<Matrix<dim>> {
	static constexpr bool isVector = false;
	static constexpr bool isMatrix = true;
	static constexpr size_t dimension = dim;
};
}// namespace Builder_detail
size_t Builder::AddString() {
	while (strings.size() <= stringCount) {
		strings.emplace_back();
	}
	lastStr = &strings[stringCount];
	return stringCount++;
}

void Builder::AddKernelResource(Id resource) {
	BeforeEntryHeader() << resource.ToString() << ' ';
}
void Builder::Reset(uint3 groupSize, bool useRayTracing) {
	for (auto&& i : strings) {
		i.clear();
	}
	stringCount = 0;
	beforeEntryHeaderIdx = AddString();
	afterEntryHeaderIdx = AddString();
	decorateStrIdx = AddString();
	typeStrIdx = AddString();
	AddString();
	// allocate 1m memory
	cbufferType.Delete();
	cbufferType.New();
	kernelArgType.Delete();
	kernelArgType.New();
	constArrMap.Clear();
	types.Clear();
	constMap.Clear();
	funcTypeMap.Clear();
	inBlock = false;
	constMap.Emplace(0u, Id::ZeroId());

	idCount = Id::StartId().id;
	// init
	if (useRayTracing) {
		BeforeEntryHeader() << "OpCapability RayTracingKHR\n"sv;
	} else {
		BeforeEntryHeader() = "OpCapability Shader\n"sv;
	}
	BeforeEntryHeader() << R"(OpCapability RuntimeDescriptorArray
OpCapability Matrix
OpCapability VariablePointers
)"sv;
	if (useRayTracing) {
		BeforeEntryHeader() << R"(OpExtension "SPV_KHR_ray_tracing"
)"sv;
	}
	BeforeEntryHeader() << R"(OpExtension "SPV_EXT_descriptor_indexing"
OpExtension "SPV_KHR_variable_pointers"
%49 = OpExtInstImport "GLSL.std.450"
)"sv;
	// use rayquery
	if (useRayTracing) {
		// %23 = OpTypeRayQueryKHR
		TypeStr() << R"(%28 = OpTypeAccelerationStructureKHR
%29 = OpTypePointer UniformConstant %28
%30 = OpTypePointer Function %28)"sv;
		BeforeEntryHeader() << R"(OpMemoryModel Logical GLSL450
OpEntryPoint RayGenerationKHR %48 "main" %24 %38 )"sv;
	} else {
		BeforeEntryHeader() << R"(OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %48 "main" %24 %25 %26 )"sv;
	}

	if (!useRayTracing) {
		AfterEntryHeader() += "OpExecutionMode %48 LocalSize "sv;
		vstd::to_string(groupSize.x, AfterEntryHeader());
		AfterEntryHeader() << ' ';
		vstd::to_string(groupSize.y, AfterEntryHeader());
		AfterEntryHeader() << ' ';
		vstd::to_string(groupSize.z, AfterEntryHeader());
	}
	AfterEntryHeader() << R"(
OpSource HLSL 640
OpName %48 "main"
)"sv;
	// ray payload
	Builder_detail::AddInternalTypeCode(TypeStr());
	if (useRayTracing) {
		DecorateStr() << R"(OpMemberName %36 0 "bary"
OpMemberName %36 1 "primitiveID"
OpMemberName %36 2 "instanceID"
OpDecorate %24 BuiltIn LaunchIdKHR
)"sv;
		TypeStr() << R"(%36 = OpTypeStruct %4 %2 %2
%37 = OpTypePointer RayPayloadKHR %36
)"sv;
		Str() << "%38 = OpVariable %37 RayPayloadKHR\n"sv;
	} else {
		DecorateStr() << R"(OpDecorate %24 BuiltIn GlobalInvocationId
OpDecorate %25 BuiltIn LocalInvocationId
OpDecorate %26 BuiltIn WorkgroupId
)"sv;
	}
	auto uint16{NewId()};
	constMap.Emplace(16u, uint16);
	TypeStr() << uint16.ToString() << " = OpConstant %2 16\n%32 = OpTypeArray %31 "sv << uint16.ToString() << R"(
%35 = OpTypePointer UniformConstant %32
%33 = OpTypePointer UniformConstant %31
)"sv;

	auto uint3InputPtr = GetTypeId(InternalType(InternalType::Tag::UINT, 3), PointerUsage::Input).ToString();
	string inputStr;
	inputStr << " = OpVariable "sv << uint3InputPtr << " Input\n"sv;
	{
		auto builder = Str();
		builder
			<< "%24"sv << inputStr;
		if (!useRayTracing) {
			builder
				<< "%25"sv << inputStr
				<< "%26"sv << inputStr;
		}
		builder << Id::SamplerVarId().ToString() << " = OpVariable "sv << Id::SamplerArrayPtrId().ToString() << " UniformConstant\n"sv;
	}
	BindVariable(Id::SamplerVarId(), 4, 0);
	InternalType float4Type = InternalType(InternalType::Tag::FLOAT, 4);

	TexDescriptor bdlsTex2DTypeDesc(
		float4Type,
		2,
		TexDescriptor::TexType::SampleTex);
	TexDescriptor bdlsTex3DTypeDesc(
		float4Type,
		3,
		TexDescriptor::TexType::SampleTex);
	auto bindlessTex2DType = GetRuntimeArrayTypeId(bdlsTex2DTypeDesc, PointerUsage::UniformConstant, 0);
	auto bindlessTex3DType = GetRuntimeArrayTypeId(bdlsTex3DTypeDesc, PointerUsage::UniformConstant, 0);
	//auto bufferUInt = GetTypeId(BufferTypeDescriptor(InternalType(InternalType::Tag::UINT, 1)), PointerUsage::Uniform);
	bdlsTex2D = NewId();
	bdlsTex3D = NewId();
	//bdlsBuffer{NewId()};
	Str()
		//<< bdlsBuffer.ToString() << " = OpVariable "sv << bufferUInt.ToString() << " Uniform\n"sv
		<< bdlsTex2D.ToString() << " = OpVariable "sv << bindlessTex2DType.ToString() << " UniformConstant\n"sv
		<< bdlsTex3D.ToString() << " = OpVariable "sv << bindlessTex3DType.ToString() << " UniformConstant\n"sv;
	//BindVariable(bdlsBuffer, 1, 0);
	BindVariable(bdlsTex2D, 2, 0);
	BindVariable(bdlsTex3D, 3, 0);
	//TODO: buffer b[] : 1;
	//TODO: texture2d<float4> samp[] : 2;
	//TODO: texture3d<float4> samp[] : 3;
	if (useRayTracing) {
	}
}
vstd::string const& Builder::Combine() {
	*(BeforeEntryHeader().end() - 1) = '\n';
	size_t strSize = 0;
	for (auto&& i :  vstd::ptr_range(strings.data(), stringCount)) {
		strSize += i.size();
	}
	//resultString.clear();
	auto&& resultString = strings[0];
	auto beforeSize = resultString.size();
	resultString.resize(strSize);
	auto ptr = resultString.data() + beforeSize;
	for (auto&& i :  vstd::ptr_range(strings.data() + 1, stringCount - 1)) {
		memcpy(ptr, i.data(), i.size());
		ptr += i.size();
	}
	return resultString;
}
Builder::TypeName& Builder::GetTypeName(BufferTypeDescriptor const& type) {
	auto ite = types.TryEmplace(type);
	auto& result = ite.first.Value();
	Id& typeName = result.typeId;
	typeName = Id(idCount++);
	if (!ite.second) return result;
	auto& eleType = GetTypeName(type.type);
	GenBuffer(typeName, eleType, type.type.Size());
	return result;
}

Builder::TypeName& Builder::GetTypeName(InternalType const& type) {
	auto ite = types.TryEmplace(type);
	auto& result = ite.first.Value();
	Id& typeName = result.typeId;
	if (!ite.second) return result;
	using Tag = InternalType::Tag;
	if (type.dimension <= 1) {
		switch (type.tag) {
			case Tag::BOOL:
				typeName = Id::BoolId();
				break;
			case Tag::MATRIX:
			case Tag::FLOAT:
				typeName = Id::FloatId();
				break;
			case Tag::INT:
				typeName = Id::IntId();
				break;
			case Tag::UINT:
				typeName = Id::UIntId();
				break;
		}
	} else {
		switch (type.tag) {
			case Tag::BOOL:
				typeName = Id::VecId(Id::BoolId(), type.dimension);
				break;
			case Tag::MATRIX:
				typeName = Id::MatId(type.dimension);
			case Tag::FLOAT:
				typeName = Id::VecId(Id::FloatId(), type.dimension);
				break;
			case Tag::INT:
				typeName = Id::VecId(Id::IntId(), type.dimension);
				break;
			case Tag::UINT:
				typeName = Id::VecId(Id::UIntId(), type.dimension);
				break;
		}
	}
	return result;
}
void Builder::GenBuffer(Id structId, TypeName& eleTypeName, uint arrayStride) {
	DecorateStr()
		<< "OpMemberDecorate "sv << structId.ToString() << " 0 Offset 0\nOpDecorate "sv
		<< structId.ToString() << " Block\n"sv;
	TypeStr()
		<< structId.ToString() << " = OpTypeStruct "sv
		<< GetRuntimeArrayType(eleTypeName, PointerUsage::NotPointer, arrayStride).ToString() << '\n';
}

Builder::TypeName& Builder::GetTypeName(Type const* type) {
	auto func = [&](auto& func, Type const* type) -> TypeName& {
		auto internalType = InternalType::GetType(type);
		if (internalType) {
			return GetTypeName(*internalType);
		}
		if (type->tag() == Type::Tag::BUFFER) {
			auto eleInternalType = InternalType::GetType(type);
			if (eleInternalType) {
				return GetTypeName(BufferTypeDescriptor(*eleInternalType));
			}
		} else if (type->tag() == Type::Tag::BINDLESS_ARRAY) {
			return GetTypeName(BufferTypeDescriptor(InternalType(InternalType::Tag::UINT, 1)));
		}
		auto ite = types.TryEmplace(type);
		auto& result = ite.first.Value();
		Id& typeName = result.typeId;
		if (!ite.second) return result;
		typeName = Id(idCount++);
		using Tag = Type::Tag;
		switch (type->tag()) {
			case Tag::BOOL:
				typeName = Id::BoolId();
				break;
			case Tag::FLOAT:
				typeName = Id::FloatId();
				break;
			case Tag::INT:
				typeName = Id::IntId();
				break;
			case Tag::UINT:
				typeName = Id::UIntId();
				break;
			case Tag::VECTOR: {
				typeName = Id::VecId(func(func, type->element()).typeId, type->dimension());
			} break;
			case Tag::MATRIX: {
				typeName = Id::MatId(type->dimension());
			} break;
			case Tag::ARRAY: {
				typeName = Id(idCount++);
				TypeStr()
					<< typeName.ToString() << " = OpTypeArray "sv
					<< func(func, type->element()).typeId.ToString()
					<< ' ' << GetConstId((uint)type->dimension()).ToString() << '\n';
			} break;
			case Tag::BUFFER: {
				GenBuffer(typeName, func(func, type->element()), type->element()->size());
			} break;
			case Tag::STRUCTURE: {
				typeName = GenStruct(type);
			} break;
			case Tag::ACCEL: {
				typeName = Id::AccelId();
			} break;
			default:
				assert(false);
				//TODO: texture, accel
		}
		return result;
	};
	return func(func, type);
}
Builder::TypeName& Builder::GetTypeName(TexDescriptor const& type) {
	auto ite = types.TryEmplace(type);
	auto& result = ite.first.Value();
	if (!ite.second) return result;
	auto scalarType = type.vectorType;
	scalarType.dimension = 1;
	auto scalarTypeName = GetTypeName(scalarType);
	uint sampleIdx = 0;
	vstd::string_view formatStr;
	switch (type.texType) {
		case TexDescriptor::TexType::SampleTex:
			sampleIdx = 1;
			formatStr = "Unknown"sv;
			break;
		case TexDescriptor::TexType::StorageTex:
			sampleIdx = 2;
			formatStr = Builder_detail::TexFormatName(type.vectorType);
			break;
	}
	result.typeId = NewId();
	TypeStr() << result.typeId.ToString() << " = OpTypeImage "sv << scalarTypeName.typeId.ToString() << ' ' << static_cast<char>(type.dimension + 48) << "D 2 0 0 "sv << static_cast<char>(sampleIdx + 48) << ' ' << formatStr << '\n';
	//TODO: format
	return result;
}
Id Builder::GetTypeNamePointer(TypeName& typeName, PointerUsage usage) {
	if (usage == PointerUsage::NotPointer) {
		return typeName.typeId;
	} else {
		auto& value = typeName.pointerId[(vbyte)usage - 1];
		if (!value.valid()) {
			value = Id(idCount++);
			TypeStr() << value.ToString() << " = OpTypePointer "sv << UsageName(usage) << ' ' << typeName.typeId.ToString() << '\n';
		}
		return value;
	}
}
Id Builder::GetRuntimeArrayType(TypeName& typeName, PointerUsage usage, uint runtimeArrayStride) {
	if (!typeName.runtimeId.valid()) {
		typeName.runtimeId = NewId();
		TypeStr() << typeName.runtimeId.ToString() << " = OpTypeRuntimeArray "sv << typeName.typeId.ToString() << '\n';
		if (runtimeArrayStride != 0 && typeName.runtimeArrayStride == 0) {
			typeName.runtimeArrayStride = runtimeArrayStride;
			DecorateStr()
				<< "OpDecorate "sv
				<< typeName.runtimeId.ToString()
				<< " ArrayStride "sv
				<< /*GetConstId(runtimeArrayStride).ToString()*/ vstd::to_string(runtimeArrayStride) << '\n';
		}
	}
	if (usage == PointerUsage::NotPointer)
		return typeName.runtimeId;
	auto& id = typeName.runtimePointerId[static_cast<uint>(usage) - 1];
	if (id.valid()) return id;
	id = NewId();
	TypeStr() << id.ToString() << " = OpTypePointer "sv << UsageName(usage) << ' ' << typeName.runtimeId.ToString() << '\n';
	return id;
}

Id Builder::GetTypeId(TypeDescriptor const& type, PointerUsage usage) {
	auto& typeName = type.visit_or(
		*static_cast<TypeName*>(nullptr),
		[&](auto& v) -> decltype(auto) { return GetTypeName(v); });
	return GetTypeNamePointer(typeName, usage);
}
Id Builder::GetRuntimeArrayTypeId(TypeDescriptor const& type, PointerUsage usage, uint arrayStride) {
	auto& typeName = type.visit_or(
		vstd::UndefEval<TypeName&>{},
		[&](auto& v) -> decltype(auto) { return GetTypeName(v); });
	return GetRuntimeArrayType(typeName, usage, arrayStride);
}
std::pair<Id, Id> Builder::GetTypeAndPtrId(TypeDescriptor const& type, PointerUsage usage) {
	auto& typeName = type.visit_or(
		*static_cast<TypeName*>(nullptr),
		[&](auto& v) -> decltype(auto) { return GetTypeName(v); });
	if (usage == PointerUsage::NotPointer) {
		return {typeName.typeId, typeName.typeId};
	} else {
		auto& value = typeName.pointerId[(vbyte)usage - 1];
		if (!value.valid()) {
			value = Id(idCount++);
			TypeStr() << value.ToString() << " = OpTypePointer "sv << UsageName(usage) << ' ' << typeName.typeId.ToString() << '\n';
		}
		return {typeName.typeId, value};
	}
}
Id Builder::GetSampledImageType(
	TypeName& typeName) {
	if (typeName.sampledId.valid()) return typeName.sampledId;
	typeName.sampledId = NewId();
	TypeStr() << typeName.sampledId.ToString() << " = OpTypeSampledImage "sv << typeName.typeId.ToString() << '\n';
	return typeName.sampledId;
}
Id Builder::GetSampledImageTypeId(
	TexDescriptor const& type) {
	auto&& typeName = GetTypeName(type);
	return GetSampledImageType(typeName);
}

Id Builder::GetFuncReturnTypeId(Id returnType, vstd::IRange<Id>* argType) {
	vstd::vector<uint> values;
	values.emplace_back(returnType.id);
	if (argType) {
		for (auto&& i : *argType) {
			values.emplace_back(i.id);
		}
	}
	auto md5 = vstd::MD5(
		{reinterpret_cast<vbyte const*>(values.data()),
		 values.byte_size()});
	return funcTypeMap
		.Emplace(
			md5,
			vstd::LazyEval([&] {
				Id newId(idCount++);
				auto builder = TypeStr();
				builder << newId.ToString() << " = OpTypeFunction "sv << returnType.ToString() << ' ';
				if (argType) {
					for (auto&& i : *argType) {
						builder << i.ToString() << ' ';
					}
				}
				builder << '\n';
				return newId;
			}))
		.Value();
}
std::pair<Id, size_t> Builder::GenStruct(
	vstd::IRange<
		vstd::variant<
			Type const*,
			InternalType>>& type) {
	Id structId(idCount++);
	auto size = 0;
	size_t memIdx = 0;
	auto structStr = structId.ToString();
	auto cmd = TypeStr();
	cmd << structStr << " = OpTypeStruct "sv;
	size_t maxAlign = 1;
	for (auto&& m : type) {
		size_t align = m.multi_visit_or(
			size_t(0), [](Type const* t) { return t->alignment(); }, [](InternalType const& t) { return t.Align(); });
		auto isMatrix =
			[&] { return m.multi_visit_or(
					  false,
					  [](Type const* t) { return t->is_matrix(); },
					  [](InternalType const& t) { return t.tag == InternalType::Tag::MATRIX; }); };
		auto dimension =
			[&] { return m.multi_visit_or(
					  size_t(0), [](Type const* t) { return t->dimension(); }, [](InternalType const& t) { return t.dimension; }); };
		auto structSize =
			[&] { return m.multi_visit_or(
					  size_t(0), [](Type const* t) { return t->size(); }, [](InternalType const& t) { return t.Size(); }); };
		size = vk::CalcAlign(size, align);
		maxAlign = std::max(align, maxAlign);
		DecorateStr() << "OpMemberDecorate "sv << structStr << ' ' << vstd::to_string(memIdx) << " Offset "sv << vstd::to_string(size) << '\n';
		if (isMatrix() && dimension() == 3)
			AddFloat3x3Decorate(structId, memIdx);
		size += structSize();
		cmd << GetTypeId(
				   m.visit_or(
					   TypeDescriptor{},
					   [](auto&& v) { return v; }),
				   PointerUsage::NotPointer)
				   .ToString()
			<< ' ';
		memIdx++;
	}
	size = vk::CalcAlign(size, maxAlign);

	cmd << '\n';
	return {structId, size};
}

void Builder::AddFloat3x3Decorate(Id structId, uint memberIdx) {
	DecorateStr() << "OpMemberDecorate "sv << structId.ToString() << ' ' << vstd::to_string(memberIdx) << " MatrixStride 16\n"sv;
}

Id Builder::GenStruct(Type const* type) {
	Id structId(idCount++);
	auto size = 0;
	size_t memIdx = 0;
	auto structStr = structId.ToString();
	{
		auto cmd = TypeStr();
		cmd << structStr << " = OpTypeStruct "sv;
		for (auto&& m : type->members()) {
			vk::CalcAlign(size, m->alignment());
			DecorateStr() << "OpMemberDecorate "sv << structStr << ' ' << vstd::to_string(memIdx) << " Offset "sv << vstd::to_string(size) << '\n';
			if (m->is_matrix() && m->dimension() == 3)
				AddFloat3x3Decorate(structId, memIdx);
			size += m->size();
			cmd << GetTypeId(m, PointerUsage::NotPointer).ToString() << ' ';
			memIdx++;
		}
		cmd << '\n';
	}
	return structId;
}
void Builder::BindVariable(Id varId, uint descSet, uint binding) {
	auto str = varId.ToString();
	DecorateStr()
		<< "OpDecorate "sv << str << " DescriptorSet "sv << vstd::to_string(descSet)
		<< "\nOpDecorate "sv << str << " Binding "sv << vstd::to_string(binding) << '\n';
}
void Builder::BindCBuffer(uint binding) {
	BindVariable(cbufferVar, 0, binding);
}

Id Builder::GenConstId(ConstValue const& value) {
	using namespace Builder_detail;
	auto GetScalarTypeId = [&]<typename Ele>() {
		if constexpr (std::is_same_v<Ele, uint>)
			return Id::UIntId();
		else if constexpr (std::is_same_v<Ele, int>)
			return Id::IntId();
		else if constexpr (std::is_same_v<Ele, float>)
			return Id::FloatId();
		else
			return Id::BoolId();
	};
	Id newId;
	luisa::visit([&]<typename T>(T const& v) {
		using VIF = VecInfo<T>;
		if constexpr (VIF::isVector) {
			vstd::vector<Id, VEngine_AllocType::VEngine, VIF::dimension> arr;
			auto elePtr = reinterpret_cast<VIF::elementType const*>(&v);
			arr.push_back_func(
				VIF::dimension,
				[&](size_t index) {
					return GetConstId(elePtr[index]);
				});
			auto builder = TypeStr();
			newId = NewId();
			//TODO: add type
			builder << newId.ToString() << " = OpConstantComposite "sv << VIF::VecId().ToString();
			for (auto&& i : arr) {
				builder << ' ' << i.ToString();
			}
			builder << '\n';
		} else if constexpr (VIF::isMatrix) {
			vstd::vector<Id, VEngine_AllocType::VEngine, VIF::dimension> arr;
			using EleType = decltype(v.cols[0]);
			arr.push_back_func(
				VIF::dimension,
				[&](size_t i) {
					return GetConstId(v.cols[i]);
				});
			auto builder = TypeStr();
			newId = NewId();
			builder << newId.ToString() << " = OpConstantComposite "sv << Id::MatId(VIF::dimension).ToString();
			for (auto&& i : arr) {
				builder << ' ' << i.ToString();
			}
			builder << '\n';
		} else if constexpr (std::is_same_v<T, uint>) {
			newId = NewId();
			TypeStr() << newId.ToString() << " = OpConstant "sv << Id::UIntId().ToString() << ' ' << vstd::to_string(v) << '\n';
		} else if constexpr (std::is_same_v<T, int32>) {
			newId = NewId();
			TypeStr() << newId.ToString() << " = OpConstant "sv << Id::IntId().ToString() << ' ' << vstd::to_string(v) << '\n';
		} else if constexpr (std::is_same_v<T, float>) {
			newId = NewId();
			TypeStr() << newId.ToString() << " = OpConstant "sv << Id::FloatId().ToString() << ' ' << vstd::to_string(v) << '\n';
		} else if constexpr (std::is_same_v<T, bool>) {
			newId = v ? Id::TrueId() : Id::FalseId();
		}
	},
				 value);
	return newId;
}
vstd::string_view Builder::UsageName(PointerUsage usage) {
	switch (usage) {
		case PointerUsage::StorageBuffer: {
			return "StorageBuffer"sv;
		} break;
		case PointerUsage::Input: {
			return "Input"sv;
		} break;
		case PointerUsage::Output: {
			return "Output"sv;
		} break;
		case PointerUsage::Workgroup: {
			return "Workgroup"sv;
		} break;
		case PointerUsage::Function: {
			return "Function"sv;
		} break;
		case PointerUsage::UniformConstant: {
			return "UniformConstant"sv;
		} break;
	}
	return ""sv;
}

Id Builder::GetConstId(ConstValue const& value) {
	return constMap
		.Emplace(
			value,
			vstd::LazyEval([&] -> Id {
				return GenConstId(value);
			}))
		.Value();
}
Id Builder::GenConstArrayId(ConstantData const& value, Id typeId) {
	Id newId(NewId());
	Id newPtrId(NewId());
	luisa::visit(
		[&](auto&& sp) {
			auto builder = TypeStr();
			builder << newId.ToString() << " = OpConstantComposite "sv << typeId.ToString();
			for (auto&& i : sp) {
				builder << ' ' << GetConstId(i).ToString();
			}
			builder << '\n';
		},
		value.view());
	return newId;
}

Id Builder::GetConstArrayId(ConstantData const& data, Type const* type, size_t hash) {
	return constArrMap
		.Emplace(
			hash,
			vstd::LazyEval([&] {
				return GenConstArrayId(data, GetTypeId(type, PointerUsage::NotPointer));
			}))
		.Value();
}
Id Builder::ReadSampler(Id index) {
	Id newId(NewId());
	Id acsId(NewId());
	auto builder = Str();
	builder << acsId.ToString() << " = OpAccessChain "sv << Id::SamplerPtrId().ToString() << ' ' << Id::SamplerVarId().ToString() << ' ' << index.ToString() << '\n'
			<< newId.ToString() << " = OpLoad "sv << Id::SamplerId().ToString() << ' ' << acsId.ToString() << '\n';
	return newId;
}

Component::Component(Builder* bd) : bd(bd) {}
void Builder::GenCBuffer(vstd::IRange<luisa::compute::Variable>& args) {
	auto range =
		args |
		vstd::TransformRange([&](luisa::compute::Variable const& var) -> vstd::variant<Type const*, InternalType> {
			return var.type();
		});
	std::initializer_list<InternalType> internalTypes = {
		InternalType(InternalType::Tag::UINT, 3)};
	auto addUInt3Range = vstd::CacheEndRange(internalTypes) | vstd::TransformRange([&](auto&& v) -> vstd::variant<Type const*, InternalType> {
							 return std::move(v);
						 });
	auto rangeInterface = vstd::RangeImpl(vstd::PairIterator(addUInt3Range, range));
	auto structId = GenStruct(rangeInterface);
	kernelArgType->typeId = structId.first;
	cbufferType->typeId = NewId();
	GenBuffer(cbufferType->typeId, *kernelArgType, structId.second);
	cbufferSize = structId.second;
	cbufferVar = NewId();
	AddKernelResource(cbufferVar);
	Str() << cbufferVar.ToString()
		  << " = OpVariable "sv << GetTypeNamePointer(*cbufferType, PointerUsage::StorageBuffer).ToString()
		  << ' ' << UsageName(PointerUsage::StorageBuffer)
		  << '\n';
}
void Builder::BeginFunc() {
	AddString();
	insideFunction = true;
}
void Builder::EndFunc() {
	insideFunction = false;
}
}// namespace toolhub::spv