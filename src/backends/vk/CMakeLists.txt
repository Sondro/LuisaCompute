file(GLOB_RECURSE VK_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
luisa_compute_add_backend(vk SOURCES ${VK_SOURCE})
target_include_directories(luisa-compute-backend-vk PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")

endif ()
#target_link_libraries(luisa-compute-backend-vk PRIVATE
set_target_properties(luisa-compute-backend-vk PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS OFF)
target_link_libraries(luisa-compute-backend-vk PRIVATE "C:/VulkanSDK/1.3.216.0/Lib/vulkan-1.lib")