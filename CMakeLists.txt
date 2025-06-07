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

add_executable(plot_analysis plot_analysis.cpp)
target_link_libraries(plot_analysis
    PRIVATE
        AnalysisFramework
        ${ROOT_LIBRARIES}
        ${TBB_LIBRARIES}
        ${TORCH_LIBRARIES}
)

add_executable(numucc_analysis numucc_analysis.cpp)
target_link_libraries(numucc_analysis
    PRIVATE
        AnalysisFramework
        ${ROOT_LIBRARIES}
        ${TBB_LIBRARIES}
        ${TORCH_LIBRARIES}
)

add_executable(numucc_snapshot numucc_snapshot.cpp)
target_link_libraries(numucc_snapshot
    PRIVATE
        AnalysisFramework
        ${ROOT_LIBRARIES}
        ${TBB_LIBRARIES}
        ${TORCH_LIBRARIES}
)


add_executable(signal_correlation_analysis signal_correlation_analysis.cpp)
target_link_libraries(signal_correlation_analysis
    PRIVATE
        AnalysisFramework
        ${ROOT_LIBRARIES}
        ${TBB_LIBRARIES}
        ${TORCH_LIBRARIES}
)

add_executable(save_mlp_features save_mlp_features.cpp)
target_link_libraries(save_mlp_features
    PRIVATE
        AnalysisFramework
        ${ROOT_LIBRARIES}
        ${TBB_LIBRARIES}
        ${TORCH_LIBRARIES}
)

add_executable(event_displays event_displays.cpp)
target_link_libraries(event_displays
    PRIVATE
        AnalysisFramework
        ${ROOT_LIBRARIES}
        ${TBB_LIBRARIES}
        ${TORCH_LIBRARIES}
)