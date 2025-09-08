include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(Catch2)
include(Catch)
add_executable(test_systematics test_systematics.cpp)
target_link_libraries(test_systematics PRIVATE libapp libhist libutils libsyst Eigen3::Eigen Catch2::Catch2WithMain ${ROOT_LIBRARIES})
catch_discover_tests(test_systematics)

add_executable(test_dynamic_binning test_dynamic_binning.cpp)
target_link_libraries(test_dynamic_binning PRIVATE libapp libhist libutils libsyst Eigen3::Eigen Catch2::Catch2WithMain ${ROOT_LIBRARIES})
catch_discover_tests(test_dynamic_binning)

add_executable(test_quadtree_binning test_quadtree_binning.cpp)
target_link_libraries(test_quadtree_binning PRIVATE libapp libhist libutils libsyst Eigen3::Eigen Catch2::Catch2WithMain ${ROOT_LIBRARIES})
catch_discover_tests(test_quadtree_binning)

add_executable(test_event_display_preset
  test_event_display_preset.cpp
  ${CMAKE_SOURCE_DIR}/src/presets/Presets_Plots.cpp)
target_link_libraries(test_event_display_preset PRIVATE Catch2::Catch2WithMain nlohmann_json::nlohmann_json)
catch_discover_tests(test_event_display_preset)

add_executable(test_pipeline_builder test_pipeline_builder.cpp)
target_link_libraries(test_pipeline_builder PRIVATE libapp libhist libutils libsyst Eigen3::Eigen Catch2::Catch2WithMain ${ROOT_LIBRARIES})
catch_discover_tests(test_pipeline_builder)