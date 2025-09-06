add_library(libhist VectorStratifier.cpp)

target_include_directories(libhist
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(libhist
  PUBLIC
    libutils
)
