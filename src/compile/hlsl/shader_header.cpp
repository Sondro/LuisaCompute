#include <compile/hlsl/shader_header.h>
#include <vstl/BinaryReader.h>
namespace toolhub::directx {
vstd::string ReadInternalHLSLFile(vstd::string_view shaderName, std::filesystem::path const &dataPath) {
    BinaryReader reader((dataPath / shaderName).string().c_str());
    vstd::string str;
    if (reader) {
        str.resize(reader.GetLength());
        reader.Read(str.data(), str.size());
    }
    return str;
}
}// namespace toolhub::directx