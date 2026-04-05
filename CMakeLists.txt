cmake_minimum_required(VERSION 3.16)
project(QObjectLeakTracker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core CorePrivate)

add_library(QObjectLeakTracker STATIC
    src/QObjectLeakTracker.h
    src/QObjectLeakTracker.cpp
)

target_include_directories(QObjectLeakTracker PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(QObjectLeakTracker PUBLIC Qt6::Core PRIVATE Qt6::CorePrivate)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    add_subdirectory(example)
    enable_testing()
    add_subdirectory(tests)
endif()
