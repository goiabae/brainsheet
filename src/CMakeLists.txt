cmake_minimum_required(VERSION 3.22.2)

project(brainsheet)

set_property(GLOBAL PROPERTY C_STANDARD 11)

set(CMAKE_C_FLAGS_DEBUG "-g3 -fsanitize=address,undefined")
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -fsanitize=address,undefined")

add_compile_options(-Wall -Wextra -Wpedantic)

add_executable(bs bs.c)

# enable_testing()
# add_test(test test.c)
