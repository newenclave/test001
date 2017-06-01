
cmake_minimum_required(VERSION 2.8)

set(INTERNAL_PROJECT_NAME test)

project(${INTERNAL_PROJECT_NAME})

find_package( Protobuf REQUIRED)

include_directories( ${INCLUDE_DIRS} )

add_executable( test "main.cpp" )

target_link_libraries( test ${Boost_LIBRARIES} )

