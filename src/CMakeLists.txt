cmake_minimum_required(VERSION 3.2)
project(WISCANET VERSION 0.7.1)

# Configure version string (verStr.h)

# Configure Global Compile Options
add_compile_options(-Wall -std=c++17 -Werror=format-security)

include_directories(include)

add_subdirectory(cnode)
add_subdirectory(enode)
