cmake_minimum_required(VERSION 3.18)

project(example LANGUAGES C)

find_package(PostgreSQL REQUIRED)

add_executable(example example.c)
target_link_libraries(example PRIVATE PostgreSQL::PostgreSQL)
if(NOT MSVC)
    target_compile_options(example PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
