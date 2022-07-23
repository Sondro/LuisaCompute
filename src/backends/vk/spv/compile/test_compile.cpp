#include "test_codegen.hpp"
#include <vstl/string_builder.h>
#include <vstl/ranges.h>
struct SetSpvCompilerAlloc {
	SetSpvCompilerAlloc() {
		spvstd::set_malloc_func(vengine_malloc);
		spvstd::set_free_func(vengine_free);
	}
};
static SetSpvCompilerAlloc gSetSpvCompilerAlloc;
void PrintHLSL() {
	spvtools::SpirvTools core(SPV_ENV_UNIVERSAL_1_3);
	spvtools::Optimizer opt(SPV_ENV_UNIVERSAL_1_3);
	toolhub::directx::DXShaderCompiler dxc;

	BinaryReader reader("hlsl_example.hlsl");
	vstd::string strv;
	strv.resize(reader.GetLength());
	reader.Read(strv.data(), strv.size());
	auto result = dxc.CompileCompute(
		strv, false);
	result.multi_visit(
		[&](vstd::unique_ptr<toolhub::directx::DXByteBlob> const& blob) {
			spvstd::vector<uint32_t> spirv;
			spirv.resize(blob->GetBufferSize() / sizeof(uint32_t));
			memcpy(spirv.data(), blob->GetBufferPtr(), spirv.size() * sizeof(uint32_t));
			vstd::string disassembly;
			if (!core.Disassemble(spirv, &disassembly)) return;
			auto f = fopen("output.spvasm", "wb");
			if (f) {
				fwrite(disassembly.data(), disassembly.size(), 1, f);
				fclose(f);
			}
			std::cout << "Success\n";
		},
		[&](vstd::string const& str) {
			std::cout << str << '\n';
		});
}

int main() {
	auto func = [&] {
		std::cout << R"(0: hlsl to spv
1: spv compile
2: test print
3:exit
)"sv;
		vstd::string cmd;
		std::cin >> cmd;
		if (cmd == "3") return 2;
		if (cmd == "0") {
			PrintHLSL();
			return 0;
		} else if (cmd == "2") {
			TestCompile();
			return 0;
		} else {
			BinaryReader reader("example.spvasm");

			spvtools::SpirvTools core(SPV_ENV_UNIVERSAL_1_3);
			spvtools::Optimizer opt(SPV_ENV_UNIVERSAL_1_3);
			vstd::string strv;
			strv.resize(reader.GetLength());
			reader.Read(strv.data(), strv.size());
			auto print_msg_to_stderr = [](spv_message_level_t, const char*,
										  const spv_position_t&, const char* m) {
				std::cerr << "error: " << m << std::endl;
			};
			core.SetMessageConsumer(print_msg_to_stderr);
			opt.SetMessageConsumer(print_msg_to_stderr);

			spvstd::vector<uint32_t> spirv;
			if (!core.Assemble(strv, &spirv)) return 1;
			if (!core.Validate(spirv)) return 1;
			std::cout << "compile success\n";
			return 0;
			opt.RegisterPass(spvtools::CreateFreezeSpecConstantValuePass())
				.RegisterPass(spvtools::CreateUnifyConstantPass())
				.RegisterPass(spvtools::CreateStripDebugInfoPass());
			if (!opt.Run(spirv.data(), spirv.size(), &spirv)) return 1;

			spvstd::string disassembly;
			if (!core.Disassemble(spirv, &disassembly)) return 1;
			std::cout << disassembly << "\n";

			return 0;
		}
	};
	while (func() != 2) {
	}
	return 0;
}
