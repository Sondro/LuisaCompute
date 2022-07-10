#pragma once
#include <spirv-tools/libspirv.hpp>
#include <spirv-tools/optimizer.hpp>
#include <dxc/dxc_util.h>
#include <vstl/BinaryReader.h>
#include <spv/codegen/builder.h>
#include <spv/codegen/function.h>
#include <spv/codegen/variable.h>
#include <spv/codegen/if_branch.h>
#include <spv/codegen/while_loop.h>
#include <spv/codegen/switch_case.h>
#include <spv/codegen/ray_query.h>
using namespace toolhub::spv;
void FunctionMain(Builder& bd) {
	//Function f(bd, )
}
void TestCompile() {
	Builder bd;
	bd.Reset(uint3(8, 8, 2), true);
	{
		Function mainFunc(&bd);
		std::initializer_list<InternalType> typeIds{
			InternalType(InternalType::Tag::FLOAT, 2),
			InternalType(InternalType::Tag::FLOAT, 3),
			InternalType(InternalType::Tag::MATRIX, 3),
			InternalType(InternalType::Tag::UINT, 4)};
		bd.GenStruct(typeIds);

	}
	vstd::string disassembly = bd.Combine();
	auto f = fopen("output.spvasm", "wb");
	if (f) {
		fwrite(disassembly.data(), disassembly.size(), 1, f);
		fclose(f);
	}
}