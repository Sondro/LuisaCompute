add_library(luisa-compute-ext SHARED dummy.cpp)
set_target_properties(luisa-compute-ext PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(FMT_OS OFF CACHE BOOL "" FORCE)
add_subdirectory(fmt)
target_compile_definitions(fmt-header-only INTERFACE
        FMT_CONSTEVAL=constexpr
        FMT_EXCEPTIONS=0
        FMT_HEADER_ONLY=1
        FMT_USE_NOEXCEPT=1)
target_link_libraries(luisa-compute-ext PUBLIC fmt::fmt-header-only)

set(SPDLOG_ENABLE_PCH OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_SHARED ON CACHE BOOL "" FORCE)
set(SPDLOG_NO_EXCEPTIONS ON CACHE BOOL "" FORCE)
set(SPDLOG_NO_THREAD_ID ON CACHE BOOL "" FORCE)
set(SPDLOG_FMT_EXTERNAL_HO ON CACHE BOOL "" FORCE)
set(SPDLOG_DISABLE_DEFAULT_LOGGER ON CACHE BOOL "" FORCE)
add_subdirectory(spdlog)
set_target_properties(spdlog PROPERTIES UNITY_BUILD ON)
target_link_libraries(spdlog PUBLIC fmt::fmt-header-only)
target_link_libraries(luisa-compute-ext PUBLIC spdlog)

add_library(xxhash INTERFACE)
target_include_directories(xxhash INTERFACE xxHash)
target_compile_definitions(xxhash INTERFACE XXH_INLINE_ALL)
set_target_properties(xxhash PROPERTIES UNITY_BUILD ON)
target_link_libraries(luisa-compute-ext PUBLIC xxhash)

add_library(asio INTERFACE dummy.cpp)
target_include_directories(asio INTERFACE asio/asio/include)
target_compile_definitions(asio INTERFACE
        ASIO_STANDALONE=1
        ASIO_HAS_STD_INVOKE_RESULT=1
        ASIO_NO_EXCEPTIONS=1)
if (WIN32)
    target_compile_definitions(asio INTERFACE _WIN32_WINNT=0x0A00)
    target_link_libraries(asio INTERFACE ws2_32 wsock32 bcrypt)
endif ()
target_link_libraries(luisa-compute-ext PUBLIC asio)

add_subdirectory(stb)
target_link_libraries(luisa-compute-ext PUBLIC stb)

set(JSON_MultipleHeaders ON CACHE BOOL "" FORCE)
set(JSON_SystemInclude ON CACHE BOOL "" FORCE)
add_subdirectory(json)
target_link_libraries(luisa-compute-ext PUBLIC nlohmann_json::nlohmann_json)
if (LUISA_COMPUTE_ENABLE_GUI)
    add_subdirectory(glad)
    target_link_libraries(luisa-compute-ext PUBLIC glad)

    find_package(GLFW3 CONFIG)
    if (NOT GLFW3_FOUND)
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
        set(GLFW_LIBRARY_TYPE SHARED CACHE STRING "" FORCE)
        add_subdirectory(glfw)
        set_target_properties(glfw PROPERTIES UNITY_BUILD ON)
    endif ()
    target_link_libraries(luisa-compute-ext PUBLIC glfw)

    add_subdirectory(imgui)
    target_link_libraries(luisa-compute-ext PUBLIC imgui)
endif ()
add_subdirectory(EASTL)
target_link_libraries(luisa-compute-ext PUBLIC EASTL)

set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)
add_subdirectory(abseil-cpp)
target_link_libraries(luisa-compute-ext PUBLIC absl::base absl::flat_hash_map)
target_compile_definitions(luisa-compute-ext PUBLIC ABSL_CONSUME_DLL=1)
