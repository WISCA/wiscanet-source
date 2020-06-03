cmake_minimum_required(VERSION 3.2)
project(enode)

set(SOURCE_FILES enode.cpp)
set(LIBRARIES "pthread" "tinyxml" "cnfParse")

add_compile_options(-DSUDO_MODE)

add_executable(enode ${SOURCE_FILES})
target_link_libraries(enode ${LIBRARIES})

set(UCONTROL_SOURCE_FILES uControl.cpp)
set(UCONTROL_LIBS "pthread" "boost_program_options" "uhd" "boost_thread" "boost_filesystem")

add_executable(uControl ${UCONTROL_SOURCE_FILES})
target_link_libraries(uControl ${UCONTROL_LIBS})

# Build matlab sudirectory
add_subdirectory(mat)
