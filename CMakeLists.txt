cmake_minimum_required(VERSION 3.10)
project(RareXSec LANGUAGES CXX)

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Werror -g)

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
)

add_subdirectory(libdata)
add_subdirectory(libutils)
add_subdirectory(libhist)
add_subdirectory(libplot)
add_subdirectory(libsyst)
add_subdirectory(libapp)
add_subdirectory(libplug)
add_subdirectory(tests)


set(ANALYSIS_EXECUTABLES
    analyse
)

foreach(exe ${ANALYSIS_EXECUTABLES})
    add_executable(${exe} ${exe}.cpp)
    target_link_libraries(${exe}
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
endforeach()

