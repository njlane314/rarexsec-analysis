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
  set_source_files_properties(${SRC} PROPERTIES LANGUAGE CXX)
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

add_plugin(RegionsPlugin       RegionsPlugin.h           "")
add_plugin(VariablesPlugin        VariablesPlugin.h         "")
add_plugin(StackedHistogramPlugin StackedHistogramPlugin.h  libplot)
add_plugin(EventDisplayPlugin     EventDisplayPlugin.h      libplot)
add_plugin(SelectionEfficiencyPlugin SelectionEfficiencyPlugin.h libplot)
add_plugin(UnstackedHistogramPlugin UnstackedHistogramPlugin.h libplot)
add_plugin(OccupancyMatrixPlugin OccupancyMatrixPlugin.h libplot)
add_plugin(SystematicBreakdownPlugin SystematicBreakdownPlugin.h libplot)
add_plugin(RocCurvePlugin        RocCurvePlugin.h         libplot)
add_plugin(SnapshotPlugin        SnapshotPlugin.h        "")

foreach(_p IN ITEMS RegionsPlugin VariablesPlugin StackedHistogramPlugin EventDisplayPlugin UnstackedHistogramPlugin SelectionEfficiencyPlugin SystematicBreakdownPlugin RocCurvePlugin OccupancyMatrixPlugin SnapshotPlugin)

  set_target_properties(${_p} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
  )
endforeach()
