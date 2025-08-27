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

function(add_plugin NAME HEADER CLASS EXTRA_LIBS)
  add_library(${NAME} SHARED PluginFactory.cpp)
  target_include_directories(${NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
  )
  target_compile_definitions(${NAME} PRIVATE
    PLUGIN_HEADER="${HEADER}"
    PLUGIN_CLASS=${CLASS}
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

add_plugin(RegionsPlugin           "RegionsPlugin.h"           analysis::RegionsPlugin           "")
add_plugin(VariablesPlugin        "VariablesPlugin.h"        analysis::VariablesPlugin         "")
add_plugin(StackedHistogramPlugin "StackedHistogramPlugin.h" analysis::StackedHistogramPlugin  libplot)
add_plugin(EventDisplayPlugin     "EventDisplayPlugin.h"     analysis::EventDisplayPlugin      libplot)
add_plugin(SelectionEfficiencyPlugin "SelectionEfficiencyPlugin.h" analysis::SelectionEfficiencyPlugin libplot)
add_plugin(UnstackedHistogramPlugin "UnstackedHistogramPlugin.h" analysis::UnstackedHistogramPlugin libplot)
add_plugin(OccupancyMatrixPlugin "OccupancyMatrixPlugin.h" analysis::OccupancyMatrixPlugin libplot)
add_plugin(SystematicBreakdownPlugin "SystematicBreakdownPlugin.h" analysis::SystematicBreakdownPlugin libplot)
add_plugin(RocCurvePlugin        "RocCurvePlugin.h"         analysis::RocCurvePlugin         libplot)
add_plugin(SnapshotPlugin        "SnapshotPlugin.h"        analysis::SnapshotPlugin        "")

foreach(_p IN ITEMS RegionsPlugin VariablesPlugin StackedHistogramPlugin EventDisplayPlugin UnstackedHistogramPlugin SelectionEfficiencyPlugin SystematicBreakdownPlugin RocCurvePlugin OccupancyMatrixPlugin SnapshotPlugin)

  set_target_properties(${_p} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
  )
endforeach()
