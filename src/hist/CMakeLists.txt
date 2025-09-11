add_library(libhist INTERFACE)

target_include_directories(libhist
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(libhist
  INTERFACE
    libutils
    libdata
    nlohmann_json::nlohmann_json
    Eigen3::Eigen
    ${ROOT_LIBRARIES}
)
