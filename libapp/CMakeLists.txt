# libapp/CMakeLists.txt

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
)

# -------------------------------------------------------------------
# Helper to build each plugin as a SHARED library from its .cpp
# -------------------------------------------------------------------
function(add_plugin NAME SRC EXTRA_LIBS)
  add_library(${NAME} SHARED ${SRC})
  target_include_directories(${NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}    # for IAnalysisPlugin.h, AnalysisDefinition.h, etc.
  )
  target_link_libraries(${NAME} PRIVATE
    libapp                         # your core INTERFACE target
    nlohmann_json::nlohmann_json   # for JSON parsing
    ${ROOT_LIBRARIES}              # for ROOT
    ${EXTRA_LIBS}                  # e.g. libplot for the stacked‐histogram plugin
  )
  set_target_properties(${NAME} PROPERTIES
    PREFIX ""                      # produce RegionsPlugin.so, not libRegionsPlugin.so
    POSITION_INDEPENDENT_CODE ON
  )
endfunction()

# -------------------------------------------------------------------
# Build each plugin from its .cpp source
# -------------------------------------------------------------------
add_plugin(RegionsPlugin           RegionsPlugin.cpp           "")
add_plugin(VariablesPlugin         VariablesPlugin.cpp         "")
add_plugin(StackedHistogramPlugin  StackedHistogramPlugin.cpp  libplot)

# put plugin .so files into the same folder as the `analyse` binary
foreach(_p IN ITEMS RegionsPlugin VariablesPlugin StackedHistogramPlugin)
  set_target_properties(${_p} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
  )
endforeach()