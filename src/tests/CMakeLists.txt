include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(Catch2)
include(Catch)
add_executable(test_systematics test_systematics.cpp)
target_link_libraries(test_systematics PRIVATE core hist utils syst Eigen3::Eigen Catch2::Catch2WithMain ${ROOT_LIBRARIES} TBB::tbb)
catch_discover_tests(test_systematics)

add_executable(test_dynamic_binning test_dynamic_binning.cpp)
target_link_libraries(test_dynamic_binning PRIVATE core hist utils syst Eigen3::Eigen Catch2::Catch2WithMain ${ROOT_LIBRARIES} TBB::tbb)
catch_discover_tests(test_dynamic_binning)

add_executable(test_quadtree_binning test_quadtree_binning.cpp)
target_link_libraries(test_quadtree_binning PRIVATE core hist utils syst Eigen3::Eigen Catch2::Catch2WithMain ${ROOT_LIBRARIES} TBB::tbb)
catch_discover_tests(test_quadtree_binning)

add_executable(test_cut_flow_summary
  test_cut_flow_summary.cpp)
target_link_libraries(test_cut_flow_summary PRIVATE Catch2::Catch2WithMain nlohmann_json::nlohmann_json ${ROOT_LIBRARIES} TBB::tbb)
catch_discover_tests(test_cut_flow_summary)

add_executable(test_cut_flow_gradient test_cut_flow_gradient.cpp)
target_link_libraries(test_cut_flow_gradient PRIVATE Catch2::Catch2WithMain nlohmann_json::nlohmann_json ${ROOT_LIBRARIES} TBB::tbb)
catch_discover_tests(test_cut_flow_gradient)

add_executable(test_monte_carlo_processor test_monte_carlo_processor.cpp)
target_link_libraries(test_monte_carlo_processor PRIVATE core hist utils syst Eigen3::Eigen Catch2::Catch2WithMain ${ROOT_LIBRARIES} TBB::tbb)
catch_discover_tests(test_monte_carlo_processor)
