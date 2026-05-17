cmake_minimum_required(VERSION 4.0)
project(DAProject2)

set(CMAKE_CXX_STANDARD 20)

add_executable(DAProject2 main.cpp
        LiveRange.h
        Web.h
        Graph.h
        MutablePriorityQueue.h
        UFDS.h
        UFDS.cpp
        RegisterPool.h
        Parser.h
        Data.h
        Parser.cpp
        menu.cpp
        menu.h
        Convert.h
        Convert.cpp)

file(COPY Data/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Data/)