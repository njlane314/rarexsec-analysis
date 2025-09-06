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

# Analysis plugins
add_plugin(EventDisplayPlugin analysis/EventDisplayPlugin.cc "")
add_plugin(OccupancyMatrixPlugin analysis/OccupancyMatrixPlugin.cc "")
add_plugin(RegionsPlugin analysis/RegionsPlugin.cc "")
add_plugin(RocCurvePlugin analysis/RocCurvePlugin.cc "")
add_plugin(SnapshotPlugin analysis/SnapshotPlugin.cc "")
add_plugin(SystematicBreakdownPlugin analysis/SystematicBreakdownPlugin.cc "")
add_plugin(UnstackedHistogramPlugin analysis/UnstackedHistogramPlugin.cc "")
add_plugin(VariablesPlugin analysis/VariablesPlugin.cc "")

# Plot plugins
add_plugin(CutFlowPlotPlugin plot/CutFlowPlotPlugin.cc "")
add_plugin(FlashValidationPlugin plot/FlashValidationPlugin.cc "")
add_plugin(RunPeriodNormalizationPlugin plot/RunPeriodNormalizationPlugin.cc "")
add_plugin(SlipStackingIntensityPlugin plot/SlipStackingIntensityPlugin.cc "")
add_plugin(StackedHistogramPlugin plot/StackedHistogramPlugin.cc "")
