add_library(libdata INTERFACE)

target_include_directories(libdata
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>   
)

target_link_libraries(libdata
  INTERFACE
    libutils
    nlohmann_json::nlohmann_json
)
