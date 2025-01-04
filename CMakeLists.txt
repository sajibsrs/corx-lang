cmake_minimum_required(VERSION 3.15.0)
project(corx VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Define the source directory
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
# Gather all source files in the src directory
file(GLOB_RECURSE SOURCES "${SRC_DIR}/*.c")

# Add the executable
add_executable(${PROJECT_NAME} main.c ${SOURCES})

# Include the src directory for headers (optional)
include_directories(${SRC_DIR})

# include(CTest)
# enable_testing()

