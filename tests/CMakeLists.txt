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

