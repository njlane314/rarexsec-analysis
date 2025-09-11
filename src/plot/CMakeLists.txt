add_library(libplot INTERFACE)

target_include_directories(libplot
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(libplot
  INTERFACE
    libdata
    libutils
    libhist
    ${ROOT_LIBRARIES}
)
