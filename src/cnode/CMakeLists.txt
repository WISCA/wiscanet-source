cmake_minimum_required(VERSION 3.2)
project(cnode)

set(SOURCE_FILES cnode.cpp)
set(LIBRARIES "pthread" "ssl" "crypto" "cnfParse" "yaml-cpp" )

add_executable(cnode ${SOURCE_FILES})
target_link_libraries(cnode ${LIBRARIES})

install(TARGETS cnode DESTINATION run/cnode/bin/)
