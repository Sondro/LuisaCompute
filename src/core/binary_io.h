#pragma once

#include <core/stl/memory.h>
#include <core/stl/string.h>

namespace luisa::compute {

class IBinaryStream {
public:
    [[nodiscard]] virtual size_t length() const = 0;
    [[nodiscard]] virtual size_t pos() const = 0;
    virtual void read(luisa::span<std::byte> dst) = 0;
	virtual ~IBinaryStream() = default;
};

class BinaryIO {

protected:
    ~BinaryIO() = default;

public:
    virtual luisa::unique_ptr<IBinaryStream> read_bytecode(luisa::string_view name) = 0;
    virtual luisa::unique_ptr<IBinaryStream> read_cache(luisa::string_view name) = 0;
    virtual void write_bytecode(luisa::string_view name, luisa::span<std::byte const> data) = 0;
    virtual void write_cache(luisa::string_view name, luisa::span<std::byte const> data) = 0;
};

}// namespace luisa::compute
