find_package(Qt6 REQUIRED COMPONENTS Test)

qt_add_executable(tst_leaktracker tst_leaktracker.cpp)
target_link_libraries(tst_leaktracker PRIVATE QObjectLeakTracker Qt6::Test)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(tst_leaktracker PRIVATE QT_DEBUG)
endif()

add_test(NAME tst_leaktracker COMMAND tst_leaktracker)
