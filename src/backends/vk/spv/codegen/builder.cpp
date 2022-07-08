#include "builder.h"
#include <vulkan_include.h>
#include "variable.h"
namespace toolhub::spv {
namespace Builder_detail {
static void GetTypeId(luisa::compute::Type const* type, vstd::string& str) {
	using Tag = luisa::compute::Type::Tag;
	switch (type->tag()) {
		case Tag::BOOL:
			str << "%bool"_sv;
			break;
		case Tag::FLOAT:
			str << "%float"_sv;
			break;
		case Tag::INT:
			str << "%int"_sv;
			break;
		case Tag::UINT:
			str << "%uint"_sv;
			break;
		case Tag::VECTOR:
			GetTypeId(type->element(), str);
			vstd::to_string(type->dimension(), str);
			break;
		case Tag::MATRIX: {
			str << "%float"_sv;
			auto dim = vstd::to_string(type->dimension());
			str << dim << 'x' << dim;
		} break;
	}
}
// float: 0
// int: 1
// uint: 2
// bool: 3
// vec: 4 + 3 * ele_type + (dim - 2)
// mat: 16 + dim - 2
// void: 19
static void AddInternalTypeCode(vstd::string& str) {
	str << R"(
%0 = = OpTypeFloat 32
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
)"_sv;
}
static void AddInternalTypeDecorate(vstd::string& str) {
	str << R"(
OpDecorate %16 ColMajor
OpDecorate %17 ColMajor
OpDecorate %18 ColMajor
OpDecorate %17 MatrixStride 16
)"_sv;
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
};
template<size_t dim>
struct VecInfo<Matrix<dim>> {
	static constexpr bool isVector = false;
	static constexpr bool isMatrix = true;
	static constexpr size_t dimension = dim;
};
}// namespace Builder_detail
void Builder::Reset(uint3 groupSize, bool useRayTracing) {
	// allocate 1m memory
	result.clear();
	decorateStr.clear();
	typeStr.clear();
	constValueStr.clear();

	result.reserve(1024 * 1024);
	decorateStr.reserve(1024);
	typeStr.reserve(4096);
	constValueStr.reserve(8192);
	types.Clear();
	constMap.Clear();
	funcTypeMap.Clear();
	idCount = Id::StartId().id;
	// init
	result = "OpCapability Shader\n"_sv;
	if (useRayTracing) {
		result += R"(OpCapability RayQueryKHR
OpExtension "SPV_KHR_ray_query"
)"_sv;
		typeStr += "%23 = OpTypeRayQueryKHR\n"_sv;
	}
	result += R"(%49 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main" %DispatchThreadID %GroupThreadID %GroupID
OpSource HLSL 630
OpExecutionMode %main LocalSize )"_sv;
	vstd::to_string(groupSize.x, result);
	result << ' ';
	vstd::to_string(groupSize.y, result);
	result << ' ';
	vstd::to_string(groupSize.z, result);
	decorateStr << R"(
OpDecorate %DispatchThreadID BuiltIn GlobalInvocationId
OpDecorate %GroupThreadID BuiltIn LocalInvocationId
OpDecorate %GroupID BuiltIn WorkgroupId
)"_sv;
	Builder_detail::AddInternalTypeDecorate(decorateStr);
	Builder_detail::AddInternalTypeCode(typeStr);
}
Builder::TypeName& Builder::GetTypeName(Type const* type) {
	auto func = [&](auto& func, Type const* type) -> TypeName& {
		auto ite = types.TryEmplace(type);
		auto& result = ite.first.Value();
		Id& typeName = result.typeId;
		if (!ite.second) return result;
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
				auto&& eleTypeName = func(func, type->element()).typeId;
				auto dim = GetConstId((uint)type->size()).ToString();
				typeName = Id(idCount++);
				typeStr << typeName.ToString() << " = OpTypeArray " << eleTypeName.ToString() << ' ' << dim << '\n';
			} break;
			case Tag::BUFFER: {
				auto&& eleTypeName = func(func, type->element()).typeId;
				auto runtimeId = Id(idCount++);
				typeName = Id(idCount++);
				auto runtimeStr = runtimeId.ToString();
				auto arrayStride = GetConstId((uint)type->element()->size()).ToString();
				decorateStr << "OpDecorate " << runtimeStr << " ArrayStride " << arrayStride << '\n';
				decorateStr << "OpMemberDecorate " << typeName.ToString() << " 0 Offset 0\nnOpDecorate"_sv
							<< typeName.ToString() << " BufferBlock\n"_sv;
				typeStr << runtimeStr
						<< " = OpTypeRuntimeArray  " << eleTypeName.ToString() << '\n';
				typeStr << typeName.ToString() << " = OpTypeStruct " << runtimeStr << '\n';
			} break;
			case Tag::STRUCTURE: {
				typeName = GenStruct(type);
			} break;
			case Tag::BINDLESS_ARRAY: {
				auto eleTypeName = Id::UIntId();
				auto runtimeId = Id(idCount++);
				typeName = Id(idCount++);
				auto runtimeStr = runtimeId.ToString();
				decorateStr << "OpDecorate " << runtimeStr << " ArrayStride " << vstd::to_string(4) << '\n';
				decorateStr << "OpMemberDecorate " << typeName.ToString() << " 0 "_sv
							<< "Offset "_sv
							<< "0\nOpDecorate "_sv
							<< typeName.ToString() << " BufferBlock\n"_sv;
				typeStr << runtimeStr
						<< " = OpTypeRuntimeArray  " << eleTypeName.ToString() << '\n';
				typeStr << typeName.ToString() << " = OpTypeStruct " << runtimeStr << '\n';
			} break;
			case Tag::ACCEL: {
				typeName = Id(idCount++);
				typeStr << typeName.ToString() << " = OpTypeAccelerationStructureKHR\n"_sv;
			} break;
				//TODO: texture, accel
		}
		return result;
	};
	return func(func, type);
}

Id Builder::GetTypeId(Type const* type, PointerUsage usage) {
	auto& typeName = GetTypeName(type);
	if (usage == PointerUsage::NotPointer) {
		return typeName.typeId;
	} else {
		auto& value = typeName.pointerId[(vbyte)usage - 1];
		if (!value.valid()) {
			value = Id(idCount++);
			typeStr << value.ToString() << " = OpTypePointer "_sv << UsageName(usage) << ' ' << typeName.typeId.ToString() << '\n';
		}
		return value;
	}
}
std::pair<Id, Id> Builder::GetTypeAndPtrId(Type const* type, PointerUsage usage) {
	auto& typeName = GetTypeName(type);
	if (usage == PointerUsage::NotPointer) {
		return {typeName.typeId, typeName.typeId};
	} else {
		auto& value = typeName.pointerId[(vbyte)usage - 1];
		if (!value.valid()) {
			value = Id(idCount++);
			typeStr << value.ToString() << " = OpTypePointer "_sv << UsageName(usage) << ' ' << typeName.typeId.ToString() << '\n';
		}
		return {typeName.typeId, value};
	}
}

Id Builder::GetFuncReturnTypeId(Id returnType, vstd::span<Id> argType) {
	return funcTypeMap
		.Emplace(
			returnType,
			vstd::MakeLazyEval([&] {
				Id newId(idCount++);
				typeStr << newId.ToString() << " = OpTypeFunction "_sv << returnType.ToString() << ' ';
				for(auto&& i : argType){
					typeStr << i.ToString() << ' ';
				}
				typeStr << '\n';
				return newId;
			}))
		.Value();
}

Id Builder::GenStruct(Type const* type) {
	Id structId(idCount++);
	auto size = 0;
	size_t memIdx = 0;
	vstd::string cmd;
	auto structStr = structId.ToString();
	cmd << structStr << " = OpTypeStruct ";
	for (auto&& m : type->members()) {
		vk::CalcAlign(size, m->alignment());
		decorateStr << "OpMemberDecorate " << structStr << vstd::to_string(memIdx) << " Offset " << vstd::to_string(size) << '\n';
		size += m->size();
		cmd << GetTypeId(m, PointerUsage::NotPointer).ToString() << ' ';
		memIdx++;
	}
	typeStr << cmd << '\n';
	return structId;
}
void Builder::BindVariable(Variable const& var, uint descSet, uint binding) {
	auto str = var.varId.ToString();
	decorateStr << "OpDecorate "_sv << str << " DescriptorSet "_sv << vstd::to_string(descSet)
				<< "\nOpDecorate "_sv << str << " Binding "_sv << vstd::to_string(binding) << '\n';
}
void Builder::GenConstId(Id id, ConstValue const& value) {
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

	value.visit([&]<typename T>(T const& v) {
		using VIF = VecInfo<T>;
		if constexpr (VIF::isVector) {
			vstd::vector<Id, VEngine_AllocType::VEngine, 4> arr;
			auto elePtr = reinterpret_cast<VIF::elementType const*>(&v);
			arr.push_back_func(
				VIF::dimension,
				[&](size_t index) {
					return GetConstId(elePtr[index]);
				});
			constValueStr << id.ToString() << " = OpConstantComposite "_sv;
			for (auto&& i : arr) {
				constValueStr << i.ToString() << ' ';
			}
			constValueStr << '\n';
		} else if constexpr (VIF::isMatrix) {
			vstd::vector<Id, VEngine_AllocType::VEngine, 4> arr;
			using EleType = decltype(v.cols[0]);
			arr.push_back_func(
				vstd::array_count(v.cols),
				[&](size_t i) {
					return GetConstId(v.cols[i]);
				});
			constValueStr << id.ToString() << " = OpConstantComposite "_sv;
			for (auto&& i : arr) {
				constValueStr << i.ToString() << ' ';
			}
			constValueStr << '\n';
		} else if constexpr (std::is_same_v<T, uint>) {
			constValueStr << id.ToString() << " = OpConstant "_sv << Id::UIntId().ToString() << ' ' << vstd::to_string(v) << '\n';
		} else if constexpr (std::is_same_v<T, int32>) {
			constValueStr << id.ToString() << " = OpConstant "_sv << Id::IntId().ToString() << ' ' << vstd::to_string(v) << '\n';
		} else if constexpr (std::is_same_v<T, float>) {
			constValueStr << id.ToString() << " = OpConstant "_sv << Id::FloatId().ToString() << ' ' << vstd::to_string(v) << '\n';
		} else if constexpr (std::is_same_v<T, bool>) {
			return v ? Id::TrueId() : Id::FalseId();
		}
	});
}
vstd::string_view Builder::UsageName(PointerUsage usage) {
	switch (usage) {
		case PointerUsage::Uniform: {
			return "Uniform"_sv;
		} break;
		case PointerUsage::Input: {
			return "Input"_sv;
		} break;
		case PointerUsage::Output: {
			return "Output"_sv;
		} break;
		case PointerUsage::Workgroup: {
			return "Workgroup"_sv;
		} break;
		case PointerUsage::Function: {
			return "Function"_sv;
		} break;
		case PointerUsage::UniformConstant: {
			return "UniformConstant"_sv;
		} break;
	}
	return ""_sv;
}

Id Builder::GetConstId(ConstValue const& value) {
	return constMap
		.Emplace(
			value,
			vstd::MakeLazyEval([&] -> Id {
				Id id(idCount++);
				GenConstId(id, value);
				return id;
			}))
		.Value();
}
Component::Component(Builder* bd) : bd(bd) {
}

}// namespace toolhub::spv