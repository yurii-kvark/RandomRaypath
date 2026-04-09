cmake_minimum_required(VERSION 4.1)

#set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
# .\msdf-atlas-gen.exe -varfont "source/GoogleSansCode-VariableFont_wght.ttf?wght=500" -type mtsdf -overlap -scanline -size 48 -pxrange 8 -potr -yorigin top -format rgba -imageout "gsanscode_w500_mtsdf.rgba" -csv "gsanscode_w500.csv"

option(RAY_ENABLE_PACKAGING "Enable distributable package generation via CPack." OFF)
option(RAY_DEBUG_NO_OPT "Enable debug build options. Also enables additional checking or stat code." OFF)

option(RAY_CPU_PROFILE "Enable profile build options." OFF)
option(RAY_GPU_PROFILE "Enable profile build options." OFF)

set(RAY_CPU_PROFILE_IMPL "tracy" CACHE STRING "CPU profiler implementation")
set(RAY_GPU_PROFILE_IMPL "tracy" CACHE STRING "GPU profiler implementation")

if(RAY_DEBUG_NO_OPT)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -fno-omit-frame-pointer -fno-inline" CACHE STRING "" FORCE)
    set(CMAKE_C_FLAGS_DEBUG   "-O0 -g3 -fno-omit-frame-pointer -fno-inline" CACHE STRING "" FORCE)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)
else()
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

project(RandomRaypath LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_MODULE_STD OFF)

option(RAY_GRAPHICS_ENABLE "Build graphics (Vulkan + GLFW), enabling graphics namespace classes." ON)

add_executable(RayClientRenderer)

if(RAY_GRAPHICS_ENABLE)
    include("${CMAKE_SOURCE_DIR}/CompileRayShaders.cmake")
    ray_enable_shader_compilation()

    add_dependencies(RayClientRenderer ${RAY_SHADERS_TARGET})
endif()

target_compile_options(RayClientRenderer
        PRIVATE
        -fno-exceptions -fno-rtti
)

# GLM
set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/glm)

# GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/glfw)

# Vulkan
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/Vulkan-Headers)

# volk
set(VOLK_PULL_IN_VULKAN ON CACHE BOOL "" FORCE)
set(VULKAN_HEADERS_INSTALL_DIR "${CMAKE_SOURCE_DIR}/third_party/Vulkan-Headers" CACHE PATH "" FORCE)

add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/volk)

# tracy

if(RAY_CPU_PROFILE AND RAY_CPU_PROFILE_IMPL STREQUAL "tracy")
    set(TRACY_ENABLE ON CACHE BOOL "" FORCE)
else()
    set(TRACY_ENABLE OFF CACHE BOOL "" FORCE)
endif()

if((RAY_CPU_PROFILE OR RAY_GPU_PROFILE) AND
(RAY_CPU_PROFILE_IMPL STREQUAL "tracy" OR RAY_GPU_PROFILE_IMPL STREQUAL "tracy"))
    set(TRACY_ON_DEMAND ON CACHE BOOL "" FORCE)
else()
    set(TRACY_ON_DEMAND OFF CACHE BOOL "" FORCE)
endif()

add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/tracy)

# Glaze (header-only JSON library)
set(glaze_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/glaze)

# Standalone Asio (header-only, vendored)
target_include_directories(RayClientRenderer
        PRIVATE
            ${CMAKE_SOURCE_DIR}/third_party/asio-asio/include)

target_compile_definitions(RayClientRenderer
        PRIVATE
            ASIO_STANDALONE
            ASIO_NO_EXCEPTIONS
            ASIO_NO_TYPEID
            ASIO_NO_DEPRECATED)

target_link_libraries(RayClientRenderer
        PRIVATE
            $<$<PLATFORM_ID:Windows>:ws2_32>
            $<$<PLATFORM_ID:Windows>:mswsock>)

# ok
target_compile_definitions(RayClientRenderer
        PRIVATE
            TOML_EXCEPTIONS=0
            RAY_GRAPHICS_ENABLE=$<IF:$<BOOL:${RAY_GRAPHICS_ENABLE}>,1,0>
            RAY_DEBUG_NO_OPT=$<IF:$<BOOL:${RAY_DEBUG_NO_OPT}>,1,0>
            RAY_CPU_PROFILE=$<IF:$<BOOL:${RAY_CPU_PROFILE}>,1,0>
            RAY_GPU_PROFILE=$<IF:$<BOOL:${RAY_GPU_PROFILE}>,1,0>
            GLM_FORCE_RADIANS
            GLM_FORCE_DEPTH_ZERO_TO_ONE)

target_include_directories(RayClientRenderer
        PRIVATE
            ${CMAKE_SOURCE_DIR}/third_party/tomlplusplus/include
            ${CMAKE_SOURCE_DIR}/third_party/glfw/deps
            ${CMAKE_SOURCE_DIR}/src)

file(GLOB_RECURSE CORE_IMPL CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)
# file(GLOB_RECURSE CORE_C CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.c)

target_sources(RayClientRenderer
        PRIVATE
            ${CORE_IMPL}
)

target_link_libraries(RayClientRenderer PRIVATE Vulkan::Headers volk::volk)
target_link_libraries(RayClientRenderer PRIVATE glfw)
target_link_libraries(RayClientRenderer PRIVATE glm::glm)
target_link_libraries(RayClientRenderer PRIVATE Tracy::TracyClient)
target_link_libraries(RayClientRenderer PRIVATE glaze::glaze)
target_link_libraries(RayClientRenderer PRIVATE stdc++exp)

if(RAY_ENABLE_PACKAGING)
    include("${CMAKE_SOURCE_DIR}/PackageRayDistribution.cmake")
    ray_enable_packaging()
endif()
