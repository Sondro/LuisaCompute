#pragma once
#include <vstl/Common.h>
#include <spv/ast/visitor.h>
#include <spv/codegen/builder.h>
#include <ast/function.h>
namespace toolhub::spv {
class SpvCompiler {
	struct CompileData {
		vstd::unique_ptr<Builder> builder;
		vstd::unique_ptr<Visitor> visitor;
	};
	vstd::vector<CompileData> builders;
	std::mutex builderMtx;
	CompileData AllocBuilder();
	void DeAllocBuilder(CompileData&& builder);
    void Compile(luisa::compute::Function func, Visitor& vis);
    void Preprocess(luisa::compute::Function func, Builder& bd);
public:
	SpvCompiler();
	~SpvCompiler();
	void CompileDebugCode(luisa::compute::Function func);
};
}// namespace toolhub::spv