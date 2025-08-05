add_library(libutils INTERFACE)

target_include_directories(libutils
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(libutils
  INTERFACE
)