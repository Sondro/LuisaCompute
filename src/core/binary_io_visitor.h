#pragma once
#include <core/stl.h>
namespace luisa::compute {
class IBinaryStream {
public:
    virtual size_t length() const = 0;
    virtual size_t pos() const = 0;
    virtual void read(luisa::span<std::byte> dst) = 0;
	virtual ~IBinaryStream() = default;
};
class BinaryIOVisitor {
protected:
    ~BinaryIOVisitor() = default;

public:
    virtual luisa::unique_ptr<IBinaryStream> read_bytecode(luisa::string_view name) = 0;
    virtual luisa::unique_ptr<IBinaryStream> read_cache(luisa::string_view name) = 0;
    virtual void write_bytecode(luisa::string_view name, luisa::span<std::byte const> data) = 0;
    virtual void write_cache(luisa::string_view name, luisa::span<std::byte const> data) = 0;
};
}// namespace luisa::compute