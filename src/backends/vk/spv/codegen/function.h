#pragma once
#include "types.h"
namespace toolhub::spv {
class Builder;
class Function : public Component {
public:
	Id func;
	Id funcType;
    Id returnType;

	Function(Builder* builder, Id returnType);
	~Function();
};
}// namespace toolhub::spv