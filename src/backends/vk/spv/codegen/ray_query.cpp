#include "ray_query.h"
#include "builder.h"
#include "function.h"
#include "if_branch.h"
namespace toolhub::spv {
void RayQuery::PrintFunc(Builder* bd) {
	///////////////// RayPayload
	auto uint_1 = bd->GetConstId(1u).ToString();
	auto maxUInt = bd->GetConstId(std::numeric_limits<uint>::max()).ToString();
	auto uintPtr = bd->GetTypeId(InternalType(InternalType::Tag::UINT, 1), PointerUsage::Function);
	auto float2Ptr = bd->GetTypeId(InternalType(InternalType::Tag::FLOAT, 2), PointerUsage::Function);
	Id rayPayload;
	Id rayPayloadPtr(bd->NewId());
	{
		auto types = {
			InternalType(InternalType::Tag::UINT, 1),
			InternalType(InternalType::Tag::UINT, 1),
			InternalType(InternalType::Tag::FLOAT, 2),
		};
		rayPayload = bd->GenStruct(types);
		bd->typeStr << rayPayloadPtr.ToString() << " = OpTypePointer Function "sv << rayPayload.ToString() << '\n';
	}
	auto GetHitResult =
		[&](
			vstd::string_view rqVar,
			vstd::string_view rayPayloadVar,
			vstd::span<Id const> args,
			uint hitFlag) -> Id {
		bd->result << rayPayloadVar << " = OpVariable "sv << rayPayloadPtr.ToString() << " Function\n";
		bd->result << rqVar << " = OpVariable "sv << Id::RayQueryPtrId().ToString() << " Function\n"sv;
		bd->result << "OpRayQueryInitializeKHR "sv
				   << rqVar << ' ' << args[0].ToString() << ' ' << bd->GetConstId(hitFlag).ToString() << ' ' << maxUInt
				   << ' ' << args[1].ToString() << ' ' << args[3].ToString() << ' ' << args[2].ToString() << ' ' << args[4].ToString() << '\n';
		Id proceed(bd->NewId());
		bd->result << proceed.ToString() << " = OpRayQueryProceedKHR "sv << Id::BoolId().ToString() << ' ' << rqVar << '\n';
		Id hitResultInt(bd->NewId());
		Id hitResultBool(bd->NewId());
		bd->result << hitResultInt.ToString() << " = OpRayQueryGetIntersectionTypeKHR "sv
				   << Id::UIntId().ToString() << ' ' << rqVar << ' ' << uint_1 << '\n'
				   << hitResultBool.ToString()
				   << " = OpIEqual "sv << Id::BoolId().ToString() << ' ' << hitResultInt.ToString() << ' ' << uint_1 << '\n';
		return hitResultBool;
	};

	auto argTypes = {
		Id::AccelId(),
		Id::VecId(Id::FloatId(), 3),//origin
		Id::VecId(Id::FloatId(), 3),//direction
		Id::FloatId(),				//tmin
		Id::FloatId()				//tmax
	};
	{
		static constexpr uint ANY_HIT_RAY_FLAG = 12;
		Function traceAnyFunc(bd, Id::BoolId(), argTypes);
		auto rayPayloadVar = bd->NewId().ToString();
		auto rqVar = bd->NewId().ToString();
		auto hitResult = GetHitResult(rqVar, rayPayloadVar, traceAnyFunc.argValues, ANY_HIT_RAY_FLAG);
		auto returnBranch = traceAnyFunc.GetReturnTypeBranch(hitResult);
		bd->result << "OpBranch "sv << returnBranch.ToString() << '\n';
		bd->inBlock = false;
	}
	{
		static constexpr uint CLOSEST_HIT_RAY_FLAG = 0;
		Function traceClosestFunc(bd, rayPayload, argTypes);
		auto rayPayloadVar = bd->NewId().ToString();
		auto rqVar = bd->NewId().ToString();
		auto hitResult = GetHitResult(rqVar, rayPayloadVar, traceClosestFunc.argValues, CLOSEST_HIT_RAY_FLAG);
		IfBranch hitBranch(bd, hitResult);
		auto BuildAccessChain = [&](Id chain, Id ptrType, uint num) {
			auto numId([&] {
				switch (num) {
					case 0:
						return Id::ZeroId().ToString();
					case 1:
						return uint_1;
					default:
						return bd->GetConstId(num).ToString();
				}
			}());
			bd->result << chain.ToString() << " = OpAccessChain "sv << ptrType.ToString() << ' ' << rayPayloadVar << ' ' << numId << '\n';
		};
		{
			auto trueBranch = hitBranch.TrueBranchScope();
			auto accessInstId(bd->NewId());
			auto accessPrimId(bd->NewId());
			auto accessUV(bd->NewId());

			BuildAccessChain(accessInstId, uintPtr, 0);
			BuildAccessChain(accessPrimId, uintPtr, 1);
			BuildAccessChain(accessUV, float2Ptr, 2);
			auto instId(bd->NewId());
			auto primId(bd->NewId());
			auto UV(bd->NewId());
			vstd::string endStr;
			endStr << ' ' << rqVar << ' ' << uint_1 << '\n';
			bd->result << instId.ToString() << " = OpRayQueryGetIntersectionInstanceIdKHR "sv << Id::UIntId().ToString() << endStr
					   << primId.ToString() << " = OpRayQueryGetIntersectionPrimitiveIndexKHR "sv << Id::UIntId().ToString() << endStr
					   << UV.ToString() << " = OpRayQueryGetIntersectionBarycentricsKHR "sv << Id::VecId(Id::FloatId(), 2).ToString() << endStr
					   << "OpStore "sv << accessInstId.ToString() << ' ' << instId.ToString() << '\n'
					   << "OpStore "sv << accessPrimId.ToString() << ' ' << primId.ToString() << '\n'
					   << "OpStore "sv << accessUV.ToString() << ' ' << UV.ToString() << '\n';
			auto loadId(bd->NewId());
			bd->result << loadId.ToString() << " = OpLoad "sv << rayPayload.ToString() << ' ' << rayPayloadVar << '\n';
			auto returnBranch = traceClosestFunc.GetReturnTypeBranch(loadId);
			bd->result << "OpBranch "sv << returnBranch.ToString() << '\n';
			bd->inBlock = false;
		}
		{
			auto falseBranch = hitBranch.FalseBranchScope();
			auto accessInstId(bd->NewId());
			BuildAccessChain(accessInstId, uintPtr, 0);
			auto loadId(bd->NewId());
			bd->result << "OpStore "sv << accessInstId.ToString() << ' ' << maxUInt << '\n'
					   << loadId.ToString() << " = OpLoad "sv << rayPayload.ToString() << ' ' << rayPayloadVar << '\n';
			auto returnBranch = traceClosestFunc.GetReturnTypeBranch(loadId);
			bd->result << "OpBranch "sv << returnBranch.ToString() << '\n';
			bd->inBlock = false;
		}
	}
}

}// namespace toolhub::spv

/*
color trace flag = 0
shadow trace flag = 12
hit triangle = 1
 OpRayQueryInitializeKHR %rayquery_variable %accel_value %flag %uint_4294967295 %ray_origin %t_min %dir %t_max
 %proceed = OpRayQueryProceedKHR %bool %rayquery_variable
 %hit_result_int = OpRayQueryGetIntersectionTypeKHR %uint %rayquery_variable %hit_triangle
 %hit_result_bool = OpIEqual %bool %hit_result_int %uint_1
 %instId = OpRayQueryGetIntersectionInstanceIdKHR %uint %rayquery_variable %uint_1
 %primId = OpRayQueryGetIntersectionPrimitiveIndexKHR %uint %rayquery_variable %uint_1
 %uv = OpRayQueryGetIntersectionBarycentricsKHR %v2float %rayquery_variable %uint_1

*/