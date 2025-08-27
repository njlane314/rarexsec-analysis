include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(Catch2)
include(Catch)

add_executable(histogram_uncertainty_tests histogram_uncertainty_tests.cpp)
target_link_libraries(histogram_uncertainty_tests PRIVATE libapp libhist libutils Eigen3::Eigen ${ROOT_LIBRARIES})
add_test(NAME histogram_uncertainty_tests COMMAND histogram_uncertainty_tests)

add_executable(test_systematics test_systematics.cpp)
target_link_libraries(test_systematics PRIVATE libapp libhist libutils libsyst Eigen3::Eigen Catch2::Catch2WithMain ${ROOT_LIBRARIES})
catch_discover_tests(test_systematics)

add_executable(stacked_histogram_uniform_binning stacked_histogram_uniform_binning.cpp)
target_link_libraries(stacked_histogram_uniform_binning PRIVATE libapp libhist libplot libutils Eigen3::Eigen ${ROOT_LIBRARIES})
add_test(NAME stacked_histogram_uniform_binning COMMAND stacked_histogram_uniform_binning)

add_executable(stacked_histogram_underflow_overflow stacked_histogram_underflow_overflow.cpp)
target_link_libraries(stacked_histogram_underflow_overflow PRIVATE libapp libhist libplot libutils Eigen3::Eigen ${ROOT_LIBRARIES})
add_test(NAME stacked_histogram_underflow_overflow COMMAND stacked_histogram_underflow_overflow)

add_executable(histogram_property_tests histogram_property_tests.cpp)
target_link_libraries(histogram_property_tests PRIVATE libapp libhist libutils Eigen3::Eigen Catch2::Catch2WithMain ${ROOT_LIBRARIES})
catch_discover_tests(histogram_property_tests)
