find_package(Qt6 REQUIRED COMPONENTS Core)

add_executable(LeakTrackerExample main.cpp)
target_link_libraries(LeakTrackerExample PRIVATE QObjectLeakTracker Qt6::Core)
target_compile_definitions(LeakTrackerExample PRIVATE QT_DEBUG)
