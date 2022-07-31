#include "spv_compiler.h"
#include <spirv-tools/libspirv.hpp>
#include <spirv-tools/optimizer.hpp>
namespace toolhub::spv {
SpvCompiler::SpvCompiler() {}
SpvCompiler::CompileData SpvCompiler::AllocBuilder() {
	std::lock_guard lck(builderMtx);
	auto bd = new Builder();
	if (builders.empty()) return {vstd::create_unique(bd), vstd::create_unique(new Visitor(bd))};
	return builders.erase_last();
}
void SpvCompiler::DeAllocBuilder(CompileData&& builder) {
	std::lock_guard lck(builderMtx);
	builders.emplace_back(std::move(builder));
}
void SpvCompiler::CompileDebugCode(luisa::compute::Function func) {
	auto compData = AllocBuilder();
	auto&& builder = *compData.builder;
	auto&& visitor = *compData.visitor;
	auto disp = vstd::create_disposer([&] { DeAllocBuilder(std::move(compData)); });
	builder.Reset(func.block_size(), func.raytracing());
	visitor.kernelGroupSize = func.block_size();
	Preprocess(func, builder);
	for (auto&& i : func.custom_callables()) {
		auto func = luisa::compute::Function(i.get());
		Id retValue = builder.GetTypeId(func.return_type(), PointerUsage::NotPointer);
		auto range = vstd::IRangeImpl(
			vstd::CacheEndRange(func.arguments()) |
			vstd::TransformRange([&](luisa::compute::Variable const& var) {
				auto usage = var.tag() == luisa::compute::Variable::Tag::REFERENCE ? PointerUsage::Function : PointerUsage::NotPointer;
				return builder.GetTypeId(var.type(), usage);
			}));
		Function localFunc(&builder, retValue, &range);
		visitor.Reset(func, &localFunc);
		Compile(func, visitor);
	}
	{
		Function kernelFunc(&builder);
		visitor.Reset(func, &kernelFunc);
		Compile(func, visitor);
	}
	vstd::string strv(builder.Combine());
	auto f = fopen("output.spvasm", "wb");
	////////////////////////// Debug output
	if (f) {
		fwrite(strv.data(), strv.size(), 1, f);
		fclose(f);
	}
	spvtools::SpirvTools core(SPV_ENV_UNIVERSAL_1_3);
	spvtools::Optimizer opt(SPV_ENV_UNIVERSAL_1_3);
	spvstd::vector<uint32_t> spirv;
	auto print_msg_to_stderr = [](spv_message_level_t, const char*,
								  const spv_position_t&, const char* m) {
		std::cerr << "error: " << m << std::endl;
	};
	core.SetMessageConsumer(print_msg_to_stderr);
	opt.SetMessageConsumer(print_msg_to_stderr);
	if (!core.Assemble(strv, &spirv) || !core.Validate(spirv)) {
		return;
	}
	spvstd::string disassembly;
	if (!core.Disassemble(spirv, &disassembly)) return;
	std::cout << "compile success\n";
}
void SpvCompiler::Compile(luisa::compute::Function func, Visitor& vis) {
	func.body()->accept(vis);
}
void SpvCompiler::Preprocess(luisa::compute::Function func, Builder& bd) {
	auto GenRange = [&](bool isLocal) {
		return vstd::CacheEndRange(func.arguments()) |
			   vstd::FilterRange([=](luisa::compute::Variable const& var) {
				   using Tag = luisa::compute::Variable::Tag;
				   bool isLocalValue = var.tag() == Tag::LOCAL || var.tag() == Tag::REFERENCE;
				   return isLocal ? isLocalValue : !isLocalValue;
			   }) |
			   vstd::RemoveCVRefRange{};
	};
	auto range = GenRange(true);
	auto irange = vstd::IRangeImpl(range);
	bd.GenCBuffer(irange);
	bd.BindCBuffer(0);
	uint bindPos = 1;
	auto propRange = GenRange(false);
	//TODO: properties
}
SpvCompiler::~SpvCompiler() {}
}// namespace toolhub::spv