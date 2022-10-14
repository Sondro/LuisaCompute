#include <vstl/BinaryReader.h>
#ifdef _WIN32
#define VSTD_FSEEK _fseeki64_nolock
#define VSTD_FTELL _ftelli64_nolock
#else
#define VSTD_FSEEK fseek
#define VSTD_FTELL ftell
#endif
BinaryReader::BinaryReader(vstd::string const &path) {
    currentPos = 0;
    ifs = fopen(path.c_str(), "rb");
    isAvaliable = ifs;
    if (isAvaliable) {
        VSTD_FSEEK(ifs, 0, SEEK_END);
        length = VSTD_FTELL(ifs);
    } else {
        length = 0;
    }
}
void BinaryReader::Read(void *ptr, uint64 len) {
    if (!isAvaliable) return;
    uint64 targetEnd = currentPos + len;
    if (targetEnd > length) {
        targetEnd = length;
        len = targetEnd - currentPos;
    }
    auto lastPos = currentPos;
    currentPos = targetEnd;
    if (len == 0) return;
    VSTD_FSEEK(ifs, lastPos, SEEK_SET);
    fread(ptr, len, 1, ifs);
}
vstd::string BinaryReader::ReadToString() {
    if (!isAvaliable) return {};
    auto len = GetLength();
    uint64 targetEnd = currentPos + len;
    if (targetEnd > length) {
        targetEnd = length;
        len = targetEnd - currentPos;
    }
    auto lastPos = currentPos;
    currentPos = targetEnd;
    if (len == 0) {};
    vstd::string str;
    str.resize(len);
    VSTD_FSEEK(ifs, lastPos, SEEK_SET);
    fread(str.data(), len, 1, ifs);
    return str;
}

luisa::vector<uint8_t> BinaryReader::Read(bool addNullEnd) {
    if (!isAvaliable) return luisa::vector<uint8_t>();
    auto len = length;
    auto lastPos = currentPos;
    uint64 targetEnd = currentPos + len;
    if (targetEnd > length) {
        targetEnd = length;
        len = targetEnd - currentPos;
    }
    if (len == 0) {
        if (addNullEnd)
            return luisa::vector<uint8_t>({0});
        else
            return luisa::vector<uint8_t>();
    }
    luisa::vector<uint8_t> result;
    result.resize(addNullEnd ? (len + 1) : len);
    currentPos = targetEnd;
    VSTD_FSEEK(ifs, lastPos, SEEK_SET);
    fread(result.data(), len, 1, ifs);
    return result;
}
BinaryReader::~BinaryReader() {
    if (!isAvaliable) return;
    fclose(ifs);
}

#undef VSTD_FSEEK
#undef VSTD_FTELL
