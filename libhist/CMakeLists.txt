add_library(libhist INTERFACE)

target_include_directories(libhist
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>   
)

target_link_libraries(libhist
  INTERFACE
    libutils
)