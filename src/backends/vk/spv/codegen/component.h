#pragma once
#include "types.h"
namespace toolhub::spv {
class Builder;
class Component : public vstd::IOperatorNewBase {
protected:
	Builder* builder;

public:
	Component(Builder* b) : builder(b) {}
	virtual ~Component() = default;
};
}// namespace toolhub::spv