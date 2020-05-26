cmake_minimum_required(VERSION 3.2)
project(WISCANET)

# Configure version string (verStr.h)

# Configure Global Compile Options
add_compile_options(-Wall -std=c++17 -Werror=format-security)

add_subdirectory(cnode)
add_subdirectory(enode)
