CMAKE_MINIMUM_REQUIRED(VERSION 3.12.1)
# require c++ 11
SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_EXTENSIONS OFF)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()
# 设置目标文件\
file(GLOB_RECURSE CPP_SRC_LIST ${CMAKE_CURRENT_LIST_DIR}/test/*.cpp)
foreach(v ${CPP_SRC_LIST})
    string(REGEX MATCH "test/.*" relative_path ${v})
    string(REGEX REPLACE "test/" "" target_name ${relative_path})
    string(REGEX REPLACE ".cpp" "" target_name ${target_name})
    add_executable(${target_name}_test ${v} ${ECACHE_SOURCE})
    target_link_libraries(${target_name}_test
            event
            pthread)
    add_test(NAME ${target_name}_test COMMAND ${target_name}_test
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
endforeach()