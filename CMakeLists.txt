cmake_minimum_required(VERSION 4.1)
project(RandRaypath)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-fno-exceptions -fno-rtti)

add_executable(RandRaypath main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE "-lstdc++exp")