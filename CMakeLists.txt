cmake_minimum_required(VERSION 3.16)
project(PageVault LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(PageVault
    src/main.cpp
    src/database.cpp
    src/memory.cpp
)
