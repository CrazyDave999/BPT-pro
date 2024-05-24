cmake_minimum_required(VERSION 3.22)
project(BPT_pro)


set(CMAKE_CXX_STANDARD 23)
#if(NOT CMAKE_BUILD_TYPE)
#    set(CMAKE_BUILD_TYPE Release)
#endif()
#set(CMAKE_CXX_FLAGS_RELEASE -Ofast)
#
#set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -O3  -Wall")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3  -Wall")
set(SRC_LIST main.cpp)
include_directories(include)
add_executable(${PROJECT_NAME} ${SRC_LIST})

add_subdirectory(src)