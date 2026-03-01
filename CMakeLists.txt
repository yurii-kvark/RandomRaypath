cmake_minimum_required(VERSION 4.1)

#set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
# .\msdf-atlas-gen.exe -varfont "source/GoogleSansCode-VariableFont_wght.ttf?wght=500" -type mtsdf -overlap -scanline -size 48 -pxrange 8 -potr -yorigin top -format rgba -imageout "gsanscode_w500_mtsdf.rgba" -csv "gsanscode_w500.csv"
# must be -pxrange 8
option(RAY_DEBUG_NO_OPT "Enable debug build options." ON)

if(RAY_DEBUG_NO_OPT)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -fno-omit-frame-pointer -fno-inline" CACHE STRING "" FORCE)
    set(CMAKE_C_FLAGS_DEBUG   "-O0 -g3 -fno-omit-frame-pointer -fno-inline" CACHE STRING "" FORCE)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)
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

# ok
target_compile_definitions(RayClientRenderer
        PRIVATE
            TOML_EXCEPTIONS=0
            RAY_GRAPHICS_ENABLE=$<IF:$<BOOL:${RAY_GRAPHICS_ENABLE}>,1,0>
            RAY_DEBUG_NO_OPT=$<IF:$<BOOL:${RAY_DEBUG_NO_OPT}>,1,0>
            GLM_FORCE_RADIANS
            GLM_FORCE_DEPTH_ZERO_TO_ONE)

target_include_directories(RayClientRenderer
        PRIVATE
            ${CMAKE_SOURCE_DIR}/third_party/tomlplusplus/include
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
target_link_libraries(RayClientRenderer PRIVATE stdc++exp)