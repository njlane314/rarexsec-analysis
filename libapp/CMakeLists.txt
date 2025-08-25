add_library(libapp INTERFACE)

target_include_directories(libapp
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(libapp
  INTERFACE
    libdata
    libutils
    libhist
    Eigen3::Eigen
    nlohmann_json::nlohmann_json
    ${ROOT_LIBRARIES}
)

