#pragma vengine_package vengine_dll
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
        VSTD_FSEEK(ifs, 0, SEEK_SET);
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
    currentPos = targetEnd;
    if (len == 0) return;
    fread(ptr, len, 1, ifs);
}
eastl::vector<uint8_t> BinaryReader::Read(bool addNullEnd) {
    if (!isAvaliable) return eastl::vector<uint8_t>();
    auto len = length;
    uint64 targetEnd = currentPos + len;
    if (targetEnd > length) {
        targetEnd = length;
        len = targetEnd - currentPos;
    }
    if (len == 0) {
        if (addNullEnd)
            return eastl::vector<uint8_t>({0});
        else
            return eastl::vector<uint8_t>();
    }
    eastl::vector<uint8_t> result;
    result.resize(addNullEnd ? (len + 1) : len);
    currentPos = targetEnd;
    fread(result.data(), len, 1, ifs);
    return result;
}

void BinaryReader::SetPos(uint64 pos) {
    if (!isAvaliable) return;
    if (pos > length) pos = length;
    currentPos = pos;
    VSTD_FSEEK(ifs, currentPos, SEEK_SET);
}
BinaryReader::~BinaryReader() {
    if (!isAvaliable) return;
    fclose(ifs);
}
#undef VSTD_FSEEK _fseeki64_nolock
#undef VSTD_FTELL _ftelli64_nolock