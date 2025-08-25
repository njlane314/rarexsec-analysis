add_library(libplot
  SystematicBreakdownPlot.cpp
)

target_include_directories(libplot
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(libplot
  PUBLIC
    libdata
    libutils
    ${ROOT_LIBRARIES}
)