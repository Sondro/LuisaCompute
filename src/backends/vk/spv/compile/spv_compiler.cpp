#include "spv_compiler.h"
#include <spirv-tools/libspirv.hpp>
#include <spirv-tools/optimizer.hpp>
#include <spv/codegen/ray_query.h>
#include <vstl/BinaryReader.h>
namespace toolhub::spv {
SpvCompiler::SpvCompiler() {}
SpvCompiler::CompileData SpvCompiler::AllocBuilder() {
	std::lock_guard lck(builderMtx);
	if (builders.empty()) {
		auto bd = new Builder();
		return {vstd::create_unique(bd), vstd::create_unique(new Visitor(bd))};
	}
	return builders.erase_last();
}
void SpvCompiler::DeAllocBuilder(CompileData&& builder) {
	std::lock_guard lck(builderMtx);
	builders.emplace_back(std::move(builder));
}
vstd::string SpvCompiler::Codegen(luisa::compute::Function func) {
	auto compData = AllocBuilder();
	auto&& builder = *compData.builder;
	auto&& visitor = *compData.visitor;
	auto disp = vstd::create_disposer([&] { DeAllocBuilder(std::move(compData)); });
	const bool rayTracing = true;//func.raytracing();
	builder.Reset(func.block_size(), rayTracing);
	visitor.kernelGroupSize = func.block_size();
	visitor.isRayTracing = rayTracing;
	vstd::vector<std::pair<uint, Variable>> uniformVars;
	uint bindPos = 1;
	for (auto&& i : func.arguments()) {
		using Tag = luisa::compute::Variable::Tag;
		PointerUsage usage;
		auto func = [&] {
			auto ite = uniformVars.emplace_back(
				i.uid(),
				vstd::LazyEval([&] {
					return Variable(&builder, i.type(), usage);
				}));
			builder.AddKernelResource(ite.second.varId);
			builder.BindVariable(
				ite.second.varId,
				0,
				bindPos);
			++bindPos;
		};
		switch (i.tag()) {
			case Tag::ACCEL:
			case Tag::TEXTURE:
				usage = PointerUsage::UniformConstant;
				func();
				break;
			case Tag::BINDLESS_ARRAY:
			case Tag::BUFFER:
				usage = PointerUsage::StorageBuffer;
				func();
				break;
		}
	}
	Preprocess(func, builder);
	/*
	if (rayTracing)
		RayQuery::PrintFunc(&builder);*/

	for (auto&& i : func.custom_callables()) {
		auto func = luisa::compute::Function(i.get());
		Id retValue = builder.GetTypeId(func.return_type(), PointerUsage::NotPointer);
		auto range = vstd::RangeImpl(
			vstd::CacheEndRange(func.arguments()) |
			vstd::TransformRange([&](luisa::compute::Variable const& var) {
				auto usage = var.tag() == luisa::compute::Variable::Tag::REFERENCE ? PointerUsage::Function : PointerUsage::NotPointer;
				return builder.GetTypeId(var.type(), usage);
			}));
		Function localFunc(&builder, retValue, &range);
		visitor.Reset(func);
		visitor.func = &localFunc;
		Compile(func, visitor);
	}
	{
		visitor.Reset(func);
		for (auto&& i : uniformVars) {
			visitor.varId.Emplace(i.first, i.second);
		}
		Function kernelFunc(&builder);
		visitor.func = &kernelFunc;
		Compile(func, visitor);
	}
	return builder.Combine();
}
vstd::variant<
	spvstd::vector<uint>,
	vstd::string>
SpvCompiler::CompileCode(vstd::string const& strv, bool debug, bool optimize) {
	spvtools::SpirvTools core(SPV_ENV_UNIVERSAL_1_6);
	spvtools::Optimizer opt(SPV_ENV_UNIVERSAL_1_6);
	spvstd::vector<uint> spirv;
	vstd::string errMsg;
	auto print_msg_to_stderr = [&](spv_message_level_t, const char*, const spv_position_t&, const char* m) {
		errMsg = m;
	};
	core.SetMessageConsumer(print_msg_to_stderr);
	opt.SetMessageConsumer(print_msg_to_stderr);
	if (optimize) {
		opt.RegisterPerformancePasses();
	} else {
		opt.RegisterPass(spvtools::CreateFreezeSpecConstantValuePass())
			.RegisterPass(spvtools::CreateUnifyConstantPass())
			.RegisterPass(spvtools::CreateStripDebugInfoPass());
	}
	if (!core.Assemble(strv, &spirv) || (debug && !core.Validate(spirv))) {
		return errMsg;
	}
	if (!opt.Run(spirv.data(), spirv.size(), &spirv)) return errMsg;
	if (debug) {
		spvstd::string disassembly;
		bool disasmResult = core.Disassemble(spirv, &disassembly);
		auto f = fopen("disasm.spvasm", "wb");
		////////////////////////// Debug output
		if (f) {
			fwrite(disassembly.data(), disassembly.size(), 1, f);
			fclose(f);
		}
		if (disasmResult)
			std::cout << "compile success\n";
	}
	return spirv;
}

vstd::variant<
	spvstd::vector<uint>,
	vstd::string>
SpvCompiler::CompileSpirV(luisa::compute::Function func, bool debug, bool optimize) {

	vstd::string strv(Codegen(func));

	if (debug) {
		auto f = fopen("output.spvasm", "wb");
		////////////////////////// Debug output
		if (f) {
			fwrite(strv.data(), strv.size(), 1, f);
			fclose(f);
		}
	}
	return CompileCode(strv, debug, optimize);
}
void SpvCompiler::Compile(luisa::compute::Function func, Visitor& vis) {
	func.body()->accept(vis);
}
void SpvCompiler::Preprocess(luisa::compute::Function func, Builder& bd) {
	auto range =
		vstd::CacheEndRange(func.arguments()) |
		vstd::FilterRange([=](luisa::compute::Variable const& var) {
			using Tag = luisa::compute::Variable::Tag;
			return var.tag() == Tag::LOCAL || var.tag() == Tag::REFERENCE;
		}) |
		vstd::RemoveCVRefRange{};
	auto irange = vstd::RangeImpl(range);
	bd.GenCBuffer(irange);
	bd.BindCBuffer(0);
}
vstd::variant<
	spvstd::vector<uint>,
	vstd::string>
SpvCompiler::CompileExistsFile(luisa::compute::Function func, bool debug, bool optimize, vstd::string const& fileName) {
	vstd::string strv;
	{
		BinaryReader reader(fileName);
		strv.resize(reader.GetLength());
		reader.Read(strv.data(), strv.size());
	}
	return CompileCode(strv, debug, optimize);
}
SpvCompiler::~SpvCompiler() {}
}// namespace toolhub::spv