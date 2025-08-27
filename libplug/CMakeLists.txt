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
  )
endfunction()

add_plugin(RegionsPlugin       RegionsPlugin.cxx           "")
add_plugin(VariablesPlugin        VariablesPlugin.cxx         "")
add_plugin(StackedHistogramPlugin StackedHistogramPlugin.cxx  libplot)
add_plugin(EventDisplayPlugin     EventDisplayPlugin.cxx      libplot)
add_plugin(SelectionEfficiencyPlugin SelectionEfficiencyPlugin.cxx libplot)
add_plugin(UnstackedHistogramPlugin UnstackedHistogramPlugin.cxx libplot)
add_plugin(OccupancyMatrixPlugin OccupancyMatrixPlugin.cxx libplot)
add_plugin(SystematicBreakdownPlugin SystematicBreakdownPlugin.cxx libplot)
add_plugin(RocCurvePlugin        RocCurvePlugin.cxx         libplot)
add_plugin(SnapshotPlugin        SnapshotPlugin.cxx        "")

foreach(_p IN ITEMS RegionsPlugin VariablesPlugin StackedHistogramPlugin EventDisplayPlugin UnstackedHistogramPlugin SelectionEfficiencyPlugin SystematicBreakdownPlugin RocCurvePlugin OccupancyMatrixPlugin SnapshotPlugin)

  set_target_properties(${_p} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
  )
endforeach()
