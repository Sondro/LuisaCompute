#include "shader_header.h"
#include "vstl/BinaryReader.h"
namespace toolhub::directx {
vstd::string ReadInternalHLSLFile(vstd::string_view shaderName, vstd::string_view dataPath) {
    vstd::string path;
    path << dataPath << shaderName;
    BinaryReader reader(path);
    vstd::string str;
    if (reader) {
        str.resize(reader.GetLength());
        reader.Read(str.data(), str.size());
    } else {
		LUISA_ERROR_WITH_LOCATION("Failed to read internal shader file: {}.", path);
	}
    return str;
}
}// namespace toolhub::directx