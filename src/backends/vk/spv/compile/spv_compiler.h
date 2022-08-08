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
	vstd::string Codegen(luisa::compute::Function func);
	vstd::variant<
		spvstd::vector<uint>,
		vstd::string>
	CompileCode(vstd::string const& code, bool debug, bool optimize);

public:
	SpvCompiler();
	~SpvCompiler();
	vstd::variant<
		spvstd::vector<uint>,
		vstd::string>
	CompileSpirV(luisa::compute::Function func, bool debug, bool optimize);
	vstd::variant<
		spvstd::vector<uint>,
		vstd::string>
	CompileExistsFile(luisa::compute::Function func, bool debug, bool optimize, vstd::string const& fileName);
};
}// namespace toolhub::spv