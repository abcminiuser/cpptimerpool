cmake_minimum_required (VERSION 3.8)

add_executable (CPPTimerPool
    Test.cpp
    TimerPool.cpp
    TimerPool.hpp
)

target_include_directories (CPPTimerPool PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features (CPPTimerPool PUBLIC cxx_std_14)

if (MSVC)
    target_compile_options (CPPTimerPool PUBLIC /W3 /WX)
else ()
    target_compile_options (CPPTimerPool PUBLIC -Wall -Wextra -Werror -Wno-unused-parameter -Wshadow -Wdouble-promotion)
    target_link_libraries (CPPTimerPool PUBLIC pthread)
endif ()
