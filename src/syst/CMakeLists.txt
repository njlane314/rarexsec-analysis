add_library(libsyst INTERFACE)

target_include_directories(libsyst
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>   
)

target_link_libraries(libsyst
  INTERFACE
    libdata
    libhist
    libutils
)
