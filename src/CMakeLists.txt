cmake_minimum_required(VERSION 3.2)
project(WISCANET VERSION 0.7.1)

# Configure version string (verStr.h)
file(WRITE include/verStr.h
    "const char verStr[] = \"${WISCANET_VERSION}\";"
    )

# Configure Global Compile Options
add_compile_options(-Wall -std=c++17 -Werror=format-security)

# Add shared static libraries
add_library(cnfParse STATIC shared/cnfParse.cpp)

# Add header files
include_directories(include)

add_subdirectory(cnode)
add_subdirectory(enode)
