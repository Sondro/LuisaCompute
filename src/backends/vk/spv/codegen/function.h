#pragma once
#include "types.h"
#include <vstl/small_vector.h>
namespace toolhub::spv {
class Builder;
class Function : public Component {
public:
	Id func;
	Id funcType;
    Id returnType;
	Id funcBlockId;
	vstd::small_vector<Id> argValues;

	Function(Builder* builder, Id returnType, vstd::span<Id> argType);
	~Function();
};
}// namespace toolhub::spv
/*
color trace flag = 0
shadow trace flag = 12
hit triangle = 1
; OpRayQueryInitializeKHR %rayquery_variable %accel_value %flag %uint_4294967295 %ray_origin %t_min %dir %t_max
; %proceed = OpRayQueryProceedKHR %bool %rayquery_variable
; %hit_result_int = OpRayQueryGetIntersectionTypeKHR %uint %rayquery_variable %hit_triangle
; %hit_result_bool = OpIEqual %bool %hit_result_int %uint_1
; %instId = OpRayQueryGetIntersectionInstanceIdKHR %uint %rayquery_variable %uint_1
; %primId = OpRayQueryGetIntersectionPrimitiveIndexKHR %uint %rayquery_variable %uint_1
; %uv = OpRayQueryGetIntersectionBarycentricsKHR %v2float %rayquery_variable %uint_1
*/