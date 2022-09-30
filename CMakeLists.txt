cmake_minimum_required(VERSION 3.16)

PROJECT(cpp-socket)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)

file(GLOB SRCFILES *.h *.cpp)
add_executable(cpp-socket ${SRCFILES})
target_link_libraries(cpp-socket Threads::Threads)
