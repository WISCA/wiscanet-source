cmake_minimum_required(VERSION 3.2)
project(enode)

set(SOURCE_FILES enode.cpp)
set(LIBRARIES "pthread" "tinyxml" "cnfParse")

add_compile_options(-DSUDO_MODE)

add_executable(enode ${SOURCE_FILES})
target_link_libraries(enode ${LIBRARIES})
