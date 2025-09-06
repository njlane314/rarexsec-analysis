add_library(libplot INTERFACE)

target_include_directories(libplot
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(libplot
  INTERFACE
    libdata
    libutils
    ${ROOT_LIBRARIES}
)
