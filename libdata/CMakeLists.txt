add_library(libdata INTERFACE)

target_include_directories(libdata
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>   
)

target_link_libraries(libdata
  INTERFACE
    libutils
)