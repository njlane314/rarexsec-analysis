add_executable(test_systematics test_systematics.cpp)

target_link_libraries(test_systematics
    PRIVATE
        libsyst
        libapp
        Eigen3::Eigen
        ${ROOT_LIBRARIES}
        TBB::tbb
)

add_test(NAME test_systematics COMMAND test_systematics)
