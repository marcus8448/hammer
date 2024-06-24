cmake_minimum_required(VERSION 3.27)
project(hammer C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)

add_library(hammer SHARED main.c)

find_package(Python3 COMPONENTS Development)
target_link_libraries(hammer PUBLIC Python3::Python)
target_include_directories(hammer PUBLIC ${Python3_INCLUDE_DIRS})
target_compile_options(hammer PUBLIC -march=native -mtune=native -mavx2 -mavx -mbmi -mbmi2 -Wall -Wpedantic
        -Wno-gnu-binary-literal -fno-exceptions -fno-rtti -fno-stack-protector -fomit-frame-pointer -msse -msse2)
