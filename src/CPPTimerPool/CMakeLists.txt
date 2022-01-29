cmake_minimum_required (VERSION 3.8)

add_library (CPPTimerPool STATIC
    TimerPool.cpp
    TimerPool.hpp
)

target_include_directories (CPPTimerPool INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features (CPPTimerPool PUBLIC cxx_std_14)

if (NOT MSVC)
    target_link_libraries (CPPTimerPool PUBLIC pthread)
endif ()
