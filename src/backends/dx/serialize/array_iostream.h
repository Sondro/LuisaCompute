#pragma once
#include <vstl/Common.h>
namespace luisa::compute {
struct ArrayIStream {
	vstd::vector<std::byte> bytes;
	template<typename T>
		requires(std::is_trivial_v<T> && !std::is_pointer_v<T>)
	ArrayIStream& operator<<(T const& d) {
		auto idx = bytes.size();
		bytes.resize(idx + sizeof(T));
		memcpy(bytes.data() + idx, &d, sizeof(T));
		return *this;
	}
	ArrayIStream& operator<<(vstd::span<std::byte const> data) {
		auto idx = bytes.size();
		bytes.resize(idx + data.size_bytes());
		memcpy(bytes.data() + idx, data.data(), data.size_bytes());
		return *this;
	}
};
struct ArrayOStream {
public:
	std::byte const* ptr;
    std::byte const* end;
	template<typename T>
		requires(std::is_trivial_v<T> && !std::is_pointer_v<T>)
	ArrayOStream& operator>>(T& d) {
        assert((end - ptr) >= sizeof(T));
		memcpy(&d, ptr, sizeof(T));
		ptr += sizeof(T);
		return *this;
	}
	ArrayOStream& operator>>(vstd::span<std::byte> data) {
        assert((end - ptr) >= data.size());
		memcpy(data.data(), ptr, data.size());
		ptr += data.size();
		return *this;
	}
};

}// namespace luisa::compute