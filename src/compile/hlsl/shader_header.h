#pragma once
#include <vstl/Common.h>
#include <filesystem>
namespace toolhub::directx {
vstd::string ReadInternalHLSLFile(vstd::string_view shaderName, std::filesystem::path const &dataPath);
}// namespace toolhub::directx