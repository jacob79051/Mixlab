cmake_minimum_required(VERSION 3.10)
project(SimpleCppServer)

set(CMAKE_CXX_STANDARD 17)

add_executable(server main.cpp external/sqlite3.c)
target_include_directories(server PRIVATE external)
