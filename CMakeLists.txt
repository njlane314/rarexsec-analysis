cmake_minimum_required(VERSION 3.10)
project(RareXSecAnalysis)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_package(ROOT REQUIRED)
include(${ROOT_USE_FILE})

find_package(TBB COMPONENTS tbb REQUIRED)
find_package(Torch REQUIRED)

set(NLOHMANN_JSON_INCLUDE_DIR "/cvmfs/larsoft.opensciencegrid.org/products/nlohmann_json/v3_11_2/include")
message(STATUS "Set nlohmann_json include path to: ${NLOHMANN_JSON_INCLUDE_DIR}")

add_subdirectory(framework)
add_subdirectory(analysis) 