#pragma once
#ifdef SPV_DLL_EXPORT
#define SPV_API __declspec(dllexport)
#else
#define SPV_API __declspec(dllimport)
#endif
#ifdef SPV_DLL_EXPORT
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#else
#include <core/stl.h>
#endif
namespace spvstd {
SPV_API void set_malloc_func(void* (*funcPtr)(size_t));
SPV_API void set_free_func(void (*funcPtr)(void*));
SPV_API void* custom_allocate(size_t size);
SPV_API void custom_deallocate(void* ptr);
}// namespace spvstd
#ifdef SPV_DLL_EXPORT

namespace luisa {
template<typename T>
struct allocator {
	using value_type = T;
	constexpr allocator() noexcept = default;
	template<typename U>
	constexpr allocator(allocator<U>) noexcept {}
	[[nodiscard]] auto allocate(std::size_t n) const noexcept {
		return static_cast<T*>(spvstd::custom_allocate(sizeof(T) * n));
	}
	void deallocate(T* p, size_t) const noexcept { spvstd::custom_deallocate(p); }
	template<typename R>
	[[nodiscard]] constexpr auto operator==(allocator<R>) const noexcept -> bool {
		return std::is_same_v<T, R>;
	}
};
}// namespace luisa
#endif
namespace spvstd {
using stringstream =
	std::basic_stringstream<char, std::char_traits<char>, luisa::allocator<char>>;

template<class Kty, class Ty, class Hasher = std::hash<Kty>,
		 class Keyeq = std::equal_to<Kty>>
using unordered_map = std::unordered_map<Kty, Ty, Hasher, Keyeq,
										 luisa::allocator<std::pair<const Kty, Ty>>>;
template<class Kty, class Ty, class Pr = std::less<Kty>>
using map = std::map<Kty, Ty, Pr, luisa::allocator<std::pair<const Kty, Ty>>>;

using ostringstream =
	std::basic_ostringstream<char, std::char_traits<char>, luisa::allocator<char>>;
using istringstream =
	std::basic_istringstream<char, std::char_traits<char>, luisa::allocator<char>>;
template<typename T>
using vector = std::vector<T, luisa::allocator<T>>;
#ifdef SPV_DLL_EXPORT
using string = std::basic_string<char, std::char_traits<char>, luisa::allocator<char>>;

#else
using stringstream =
	std::basic_stringstream<char, std::char_traits<char>, luisa::allocator<char>>;
using string = luisa::string;
#endif

}// namespace spvstd
