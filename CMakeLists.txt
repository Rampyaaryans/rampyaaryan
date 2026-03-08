cmake_minimum_required(VERSION 3.10)
project(Rampyaaryan C)

# C99 standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -O2)
endif()

# Source files
set(SOURCES
    src/memory.c
    src/value.c
    src/object.c
    src/table.c
    src/chunk.c
    src/lexer.c
    src/compiler.c
    src/vm.c
    src/native.c
    src/debug.c
    src/ascii_art.c
    src/main.c
)

# Executable
add_executable(rampyaaryan ${SOURCES})

# Link math library on Unix
if(UNIX)
    target_link_libraries(rampyaaryan m)
endif()

# Debug configuration
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(rampyaaryan PRIVATE
        DEBUG_TRACE_EXECUTION
        DEBUG_STRESS_GC
    )
endif()

# Install
install(TARGETS rampyaaryan DESTINATION bin)

# Print build info
message(STATUS "Rampyaaryan Compiler - Hinglish Programming Language")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
