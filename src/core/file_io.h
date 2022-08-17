#pragma once
#include <core/stl.h>
namespace luisa::compute {
class FileIO {
protected:
	~FileIO() = default;

public:
	using AllocateFunc = luisa::move_only_function<void*(size_t byte_size)>;
	virtual luisa::span<std::byte> read_bytecode(luisa::string_view name, AllocateFunc const& alloc) = 0;
	virtual luisa::span<std::byte> read_cache(luisa::string_view name, AllocateFunc const& alloc) = 0;
	virtual void write_bytecode(luisa::string_view name, luisa::span<std::byte const> data) = 0;
	virtual void write_cache(luisa::string_view name, luisa::span<std::byte const> data) = 0;
};
}// namespace luisa::compute