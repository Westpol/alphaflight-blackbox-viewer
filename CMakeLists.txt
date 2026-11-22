cmake_minimum_required(VERSION 3.16)

project(alphaflight-blackbox-viewer
    VERSION 1.0
    LANGUAGES C
)

# C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Executable output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Source files
set(SOURCES
    src/main.c
    src/log_reader.c
    src/logging_structs.c
)

# Create executable
add_executable(blackbox ${SOURCES})

# Include directories
target_include_directories(blackbox PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Enable all warnings and treat them as errors
if (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(blackbox PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
    )
elseif (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(blackbox PRIVATE /W4 /WX)
endif()
