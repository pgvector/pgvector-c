cmake_minimum_required(VERSION 3.18)

project(pgvector LANGUAGES C)

option(BUILD_TESTING "" ON)

if(BUILD_TESTING)
    find_package(PostgreSQL REQUIRED)

    add_executable(test test/pq_test.c)
    target_link_libraries(test PRIVATE PostgreSQL::PostgreSQL)
    if(NOT MSVC)
        target_compile_options(test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endif()
