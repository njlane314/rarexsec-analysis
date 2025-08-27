include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(Catch2)
include(Catch)
add_executable(test_histogram_uncertainty test_histogram_uncertainty.cpp)
target_link_libraries(test_histogram_uncertainty PRIVATE libapp libhist libutils Eigen3::Eigen Catch2::Catch2WithMain ${ROOT_LIBRARIES})
catch_discover_tests(test_histogram_uncertainty)
add_executable(test_systematics test_systematics.cpp)
target_link_libraries(test_systematics PRIVATE libapp libhist libutils libsyst Eigen3::Eigen Catch2::Catch2WithMain ${ROOT_LIBRARIES})
catch_discover_tests(test_systematics)
add_executable(test_stacked_histogram_uniform_binning test_stacked_histogram_uniform_binning.cpp)
target_link_libraries(test_stacked_histogram_uniform_binning PRIVATE libapp libhist libplot libutils Eigen3::Eigen Catch2::Catch2WithMain ${ROOT_LIBRARIES})
catch_discover_tests(test_stacked_histogram_uniform_binning)
add_executable(test_stacked_histogram_underflow_overflow test_stacked_histogram_underflow_overflow.cpp)
target_link_libraries(test_stacked_histogram_underflow_overflow PRIVATE libapp libhist libplot libutils Eigen3::Eigen Catch2::Catch2WithMain ${ROOT_LIBRARIES})
catch_discover_tests(test_stacked_histogram_underflow_overflow)
add_executable(test_histogram_property test_histogram_property.cpp)
target_link_libraries(test_histogram_property PRIVATE libapp libhist libutils Eigen3::Eigen Catch2::Catch2WithMain ${ROOT_LIBRARIES})
catch_discover_tests(test_histogram_property)
