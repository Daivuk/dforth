cmake_minimum_required(VERSION 3.0)
project(forth_tests)
set(CMAKE_CXX_STANDARD 11)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
file(GLOB inc_files forth/*.*)
file(GLOB src_files src/*.*)
file(GLOB test_files test/*.*)
source_group("forth" FILES ${inc_files} ${src_files})
source_group("test" FILES ${test_files})
add_executable(${PROJECT_NAME} ${inc_files} ${src_files} ${test_files})
target_include_directories(${PROJECT_NAME} PRIVATE
    src
    ./
    thirdparty/catch
)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/INCLUDE.f ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/INCLUDE_INCLUDE.f ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
