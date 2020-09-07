cmake_minimum_required(VERSION 3.0)
project(forth)
file(GLOB inc_files forth/*.*)
file(GLOB src_files src/*.*)
file(GLOB test_files tests/*.*)
source_group("forth" FILES ${inc_files} ${src_files})
source_group("tests" FILES ${test_files})
add_executable(${PROJECT_NAME} ${inc_files} ${src_files} ${test_files})
target_include_directories(${PROJECT_NAME} PRIVATE
    src
    ./
    thirdparty/catch
)
