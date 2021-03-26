cmake_minimum_required(VERSION 3.2)
project(enode)

set(SOURCE_FILES enode.cpp)
set(LIBRARIES "pthread" "cnfParse" "yaml-cpp")

add_compile_options(-DSUDO_MODE)

add_executable(enode ${SOURCE_FILES})
target_link_libraries(enode ${LIBRARIES})

set(UCONTROL_SOURCE_FILES uControl.cpp)
set(UCONTROL_LIBS "pthread" "boost_program_options" "uhd" "boost_thread" "boost_filesystem" "boost_system")

add_executable(uControl ${UCONTROL_SOURCE_FILES})
target_link_libraries(uControl ${UCONTROL_LIBS})

install(TARGETS enode uControl DESTINATION run/enode/bin/)

# Build matlab sudirectory
add_subdirectory(mat)

# Build python subdirectory
add_subdirectory(python)
