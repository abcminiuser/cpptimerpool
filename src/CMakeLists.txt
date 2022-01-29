cmake_minimum_required (VERSION 3.8)

add_subdirectory("CPPTimerPool")

add_executable (TestApp
    Test.cpp
)

target_include_directories (TestApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features (TestApp PUBLIC cxx_std_14)
target_link_libraries (TestApp PRIVATE CPPTimerPool)

if (MSVC)
    target_compile_options (TestApp PUBLIC /W3 /WX)
else ()
    target_compile_options (TestApp PUBLIC -Wall -Wextra -Werror -Wno-unused-parameter -Wshadow -Wdouble-promotion)
endif ()
