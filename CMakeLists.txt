cmake_minimum_required(VERSION 4.1)

#set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")

set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -fno-omit-frame-pointer -fno-inline" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_DEBUG   "-O0 -g3 -fno-omit-frame-pointer -fno-inline" CACHE STRING "" FORCE)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)

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

add_library(glm_mod STATIC)
target_link_libraries(glm_mod PUBLIC glm::glm)
target_compile_options(glm_mod PUBLIC -fno-exceptions -fno-rtti)

set(GLM_CPPM "${CMAKE_SOURCE_DIR}/third_party/glm/glm.cppm")

target_sources(glm_mod
        PUBLIC
            FILE_SET glm_modules TYPE CXX_MODULES
            BASE_DIRS "${CMAKE_SOURCE_DIR}/third_party/glm"
            FILES "${GLM_CPPM}"
)

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

# toml++
target_include_directories(RayClientRenderer PRIVATE ${CMAKE_SOURCE_DIR}/third_party/tomlplusplus/include)
target_compile_definitions(RayClientRenderer
        PRIVATE
            TOML_EXCEPTIONS=0
            RAY_GRAPHICS_ENABLE=$<IF:$<BOOL:${RAY_GRAPHICS_ENABLE}>,1,0>
            GLM_FORCE_RADIANS
            GLM_FORCE_DEPTH_ZERO_TO_ONE)

file(GLOB_RECURSE CORE_IMPL CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE CORE_MODULES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.mpp)

target_sources(RayClientRenderer
        PRIVATE
            ${CORE_IMPL}
        PUBLIC
            FILE_SET ray_modules TYPE CXX_MODULES
            BASE_DIRS "${CMAKE_SOURCE_DIR}/src"
            FILES ${CORE_MODULES}
)

target_link_libraries(RayClientRenderer PRIVATE Vulkan::Headers volk::volk)
target_link_libraries(RayClientRenderer PRIVATE glfw)
target_link_libraries(RayClientRenderer PRIVATE glm_mod)
target_link_libraries(RayClientRenderer PRIVATE stdc++exp)