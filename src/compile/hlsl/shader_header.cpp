#include <compile/hlsl/shader_header.h>
#include <vstl/BinaryReader.h>
namespace toolhub::directx {
vstd::string ReadInternalHLSLFile(vstd::string_view shaderName, vstd::string_view dataPath) {
    vstd::string path;
    path << dataPath << shaderName;
    BinaryReader reader(path);
    vstd::string str;
    if (reader) {
        str.resize(reader.GetLength());
        reader.Read(str.data(), str.size());
    }
    return str;
}
}// namespace toolhub::directx