cmake_minimum_required(VERSION 3.10)
project(RareXSec LANGUAGES CXX)

# Use ccache for faster rebuilds when available
find_program(CCACHE_PROGRAM ccache)
if (CCACHE_PROGRAM)
    message(STATUS "Found ccache: ${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Werror -O3)

find_package(ROOT REQUIRED COMPONENTS Core Hist Tree RIO Graf)
include(${ROOT_USE_FILE})

find_package(TBB COMPONENTS tbb REQUIRED)
find_package(Torch REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(nlohmann_json REQUIRED)

include_directories(SYSTEM ${EIGEN3_INCLUDE_DIR})

include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

add_subdirectory(src/data)
add_subdirectory(src/utils)
add_subdirectory(src/hist)
add_subdirectory(src/plot)
add_subdirectory(src/syst)
add_subdirectory(src/core)
add_subdirectory(src/plug)
add_subdirectory(src/tests)
add_subdirectory(src/study)
