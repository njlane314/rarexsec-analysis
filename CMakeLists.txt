cmake_minimum_required(VERSION 3.10)
project(RareXSec LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Werror)

find_package(ROOT REQUIRED COMPONENTS Core Hist Tree RIO Graf)
include(${ROOT_USE_FILE})

find_package(TBB COMPONENTS tbb REQUIRED)
find_package(Torch REQUIRED)

set(NLOHMANN_JSON_INCLUDE_DIR "/cvmfs/larsoft.opensciencegrid.org/products/nlohmann_json/v3_11_2/include")
include_directories(SYSTEM ${NLOHMANN_JSON_INCLUDE_DIR})

add_subdirectory(libdata)
add_subdirectory(libutils)
add_subdirectory(libhist)
add_subdirectory(libplot)
add_subdirectory(libsyst)
add_subdirectory(libapp)

set(ANALYSIS_EXECUTABLES
    analyse_test
)

foreach(exe ${ANALYSIS_EXECUTABLES})
    add_executable(${exe} ${exe}.cpp)
    target_link_libraries(${exe}
        PRIVATE
            libapp
            libdata
            libhist
            libplot
            libsyst
            libutils
            ${ROOT_LIBRARIES}
            TBB::tbb
            TMVA
    )
endforeach()