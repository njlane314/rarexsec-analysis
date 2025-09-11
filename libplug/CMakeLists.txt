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

add_plugin(RegionsPlugin analytics/RegionsPlugin.cc "")
add_plugin(SnapshotPlugin analytics/SnapshotPlugin.cc "")
add_plugin(VariablesPlugin analytics/VariablesPlugin.cc "")
add_plugin(StrategySelectionPlugin systematics/StrategySelectionPlugin.cc "")

add_plugin(EventDisplayPlugin plotting/EventDisplayPlugin.cc "")
add_plugin(CutFlowPlotPlugin plotting/CutFlowPlotPlugin.cc "")
add_plugin(StackedHistogramPlugin plotting/StackedHistogramPlugin.cc "")
add_plugin(CutMatrixPlotPlugin plotting/CutMatrixPlotPlugin.cc "")
add_plugin(PerformancePlotPlugin plotting/PerformancePlotPlugin.cc "")
add_plugin(SystematicBreakdownPlugin plotting/SystematicBreakdownPlugin.cc "")
add_plugin(UnstackedHistogramPlugin plotting/UnstackedHistogramPlugin.cc "")
