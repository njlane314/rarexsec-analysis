add_library(libplug INTERFACE)

target_include_directories(libplug
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(libplug
  INTERFACE
    libapp
    libplot
    libhist
    libdata
    libutils
)

function(add_plugin NAME SRC EXTRA_LIBS)
  set_source_files_properties(${SRC} PROPERTIES LANGUAGE CXX)
  add_library(${NAME} SHARED ${SRC})
  target_compile_definitions(${NAME} PRIVATE BUILD_PLUGIN)
  target_include_directories(${NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
  )
  target_link_libraries(${NAME} PRIVATE
    libplug
    nlohmann_json::nlohmann_json
    ${ROOT_LIBRARIES}
    ${EXTRA_LIBS}
  )
  set_target_properties(${NAME} PROPERTIES
    PREFIX ""
    POSITION_INDEPENDENT_CODE ON
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
  )
endfunction()

add_plugin(EventDisplayPlugin EventDisplayPlugin.cc "")
add_plugin(OccupancyMatrixPlugin OccupancyMatrixPlugin.cc "")
add_plugin(RegionsPlugin RegionsPlugin.cc "")
add_plugin(RocCurvePlugin RocCurvePlugin.cc "")
add_plugin(SelectionEfficiencyPlugin SelectionEfficiencyPlugin.cc "")
add_plugin(SnapshotPlugin SnapshotPlugin.cc "")
add_plugin(StackedHistogramPlugin StackedHistogramPlugin.cc "")
add_plugin(SystematicBreakdownPlugin SystematicBreakdownPlugin.cc "")
add_plugin(UnstackedHistogramPlugin UnstackedHistogramPlugin.cc "")
add_plugin(VariablesPlugin VariablesPlugin.cc "")
