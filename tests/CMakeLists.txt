add_executable(histogram_uncertainty_tests histogram_uncertainty_tests.cpp)

target_link_libraries(histogram_uncertainty_tests
  PRIVATE
    libapp
    libhist
    libutils
    Eigen3::Eigen
    ${ROOT_LIBRARIES}
)

add_test(NAME histogram_uncertainty_tests COMMAND histogram_uncertainty_tests)
