cmake_minimum_required(VERSION 3.0)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
set(CMAKE_CXX_STANDARD 11)
file(GLOB inc_files forth/*.*)
file(GLOB src_files src/*.*)
file(GLOB test_files test/*.*)

function(DEFTESTPROJECT PROJECT_NAME)
    project(${PROJECT_NAME})
    source_group("forth" FILES ${inc_files} ${src_files})
    source_group("test" FILES ${test_files})
    add_executable(${PROJECT_NAME} ${inc_files} ${src_files} ${test_files})
    target_compile_definitions(${PROJECT_NAME} PRIVATE ${ARGV})
    target_include_directories(${PROJECT_NAME} PRIVATE
        src
        ./
        thirdparty/catch
    )
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/INCLUDE.f ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/INCLUDE_INCLUDE.f ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
endfunction()

deftestproject(forth_tests_8bits -DFORTH_INT_SIZE_8_BITS=1)
deftestproject(forth_tests_16bits -DFORTH_INT_SIZE_16_BITS=1)
deftestproject(forth_tests_32bits -DFORTH_INT_SIZE_32_BITS=1)
deftestproject(forth_tests_64bits -DFORTH_INT_SIZE_64_BITS=1)
project(forth_tests)
