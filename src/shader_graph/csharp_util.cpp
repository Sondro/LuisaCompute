#pragma once
#include <vstl/Common.h>
#include <core/dll_export.h>
#include <core/stl/memory.h>
LC_SHADER_GRAPH_LIB_API void *csharp_malloc(uint64_t size) {
    return luisa::detail::allocator_allocate(size, 0);
}
LC_SHADER_GRAPH_LIB_API void csharp_free(void *ptr) {
    luisa::detail::allocator_deallocate(ptr, 0);
}
LC_SHADER_GRAPH_LIB_API void *csharp_realloc(void *ptr, uint64_t size) {
    return luisa::detail::allocator_reallocate(ptr, size, 0);
}
LC_SHADER_GRAPH_LIB_API void *csharp_memcpy(void *dest, void *src, uint64_t size) {
    return memcpy(dest, src, size);
}
LC_SHADER_GRAPH_LIB_API void *csharp_memset(void *dest, int value, uint64_t size) {
    return memset(dest, value, size);
}
LC_SHADER_GRAPH_LIB_API void *csharp_memmove(void *dest, void *src, uint64_t size) {
    return memmove(dest, src, size);
}
LC_SHADER_GRAPH_LIB_API int csharp_memcmp(void *src, void *dst, uint64_t byteSize) {
    return memcmp(src, dst, byteSize);
}