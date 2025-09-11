add_library(libcore INTERFACE)

target_include_directories(libcore
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(libcore
  INTERFACE
    libdata
    libutils
    libhist
    Eigen3::Eigen
    nlohmann_json::nlohmann_json
    ${ROOT_LIBRARIES}
)

