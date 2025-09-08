cmake_minimum_required(VERSION 3.10)
project(RareXSec LANGUAGES CXX)

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
    "${CMAKE_CURRENT_SOURCE_DIR}/libapp"
    "${CMAKE_CURRENT_SOURCE_DIR}/libdata"
    "${CMAKE_CURRENT_SOURCE_DIR}/libhist"
    "${CMAKE_CURRENT_SOURCE_DIR}/libplot"
    "${CMAKE_CURRENT_SOURCE_DIR}/libsyst"
    "${CMAKE_CURRENT_SOURCE_DIR}/libutils"
    "${CMAKE_CURRENT_SOURCE_DIR}/libplug"
    "${CMAKE_CURRENT_SOURCE_DIR}/libpipe"
)

add_subdirectory(libdata)
add_subdirectory(libutils)
add_subdirectory(libhist)
add_subdirectory(libplot)
add_subdirectory(libsyst)
add_subdirectory(libapp)
add_subdirectory(libplug)
add_subdirectory(tests)

add_library(analyse STATIC src/presets/Presets_Standard.cpp src/presets/Presets_Vertices.cpp src/presets/Presets_Plots.cpp)
target_link_libraries(analyse
    PUBLIC
        libapp
        libplug
        libdata
        libhist
        libplot
        libsyst
        libutils
        Eigen3::Eigen
        nlohmann_json::nlohmann_json
        ${ROOT_LIBRARIES}
        TBB::tbb
        TMVA
        dl
)

add_executable(neutrino_vertex_stacked_example src/analysis/neutrino_vertex_stacked_plots.cpp src/presets/Presets_Standard.cpp src/presets/Presets_Vertices.cpp src/presets/Presets_Plots.cpp)
target_link_libraries(neutrino_vertex_stacked_example
    PRIVATE
        libapp
        libplug
        libdata
        libhist
        libplot
        libsyst
        libutils
        Eigen3::Eigen
        nlohmann_json::nlohmann_json
        ${ROOT_LIBRARIES}
        TBB::tbb
        TMVA
        dl
)

add_executable(pipeline_runner_example src/analysis/pipeline_runner_example.cpp src/presets/Presets_Standard.cpp src/presets/Presets_Vertices.cpp src/presets/Presets_Plots.cpp)
target_link_libraries(pipeline_runner_example
    PRIVATE
        libapp
        libplug
        libdata
        libhist
        libplot
        libsyst
        libutils
        Eigen3::Eigen
        nlohmann_json::nlohmann_json
        ${ROOT_LIBRARIES}
        TBB::tbb
        TMVA
        dl
)

