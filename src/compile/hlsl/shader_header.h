#pragma once
#include <vstl/Common.h>
#include <filesystem>
namespace toolhub::directx {
vstd::string ReadInternalHLSLFile(vstd::string_view shaderName, vstd::string_view dataPath);
}// namespace toolhub::directx