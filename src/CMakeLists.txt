add_library(utils INTERFACE)

# Common include directories for all interface libraries
set(RAREXSEC_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

# utils has no dependencies
target_include_directories(utils
  INTERFACE
    $<BUILD_INTERFACE:${RAREXSEC_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(utils
  INTERFACE
)

add_library(data INTERFACE)

target_include_directories(data
  INTERFACE
    $<BUILD_INTERFACE:${RAREXSEC_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(data
  INTERFACE
    utils
    nlohmann_json::nlohmann_json
)

add_library(hist INTERFACE)

target_include_directories(hist
  INTERFACE
    $<BUILD_INTERFACE:${RAREXSEC_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(hist
  INTERFACE
    utils
    data
    nlohmann_json::nlohmann_json
    Eigen3::Eigen
    ${ROOT_LIBRARIES}
)

add_library(plot INTERFACE)

target_include_directories(plot
  INTERFACE
    $<BUILD_INTERFACE:${RAREXSEC_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(plot
  INTERFACE
    data
    utils
    hist
    ${ROOT_LIBRARIES}
)

add_library(syst INTERFACE)

target_include_directories(syst
  INTERFACE
    $<BUILD_INTERFACE:${RAREXSEC_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(syst
  INTERFACE
    data
    hist
    utils
)

add_library(core INTERFACE)

target_include_directories(core
  INTERFACE
    $<BUILD_INTERFACE:${RAREXSEC_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(core
  INTERFACE
    data
    utils
    hist
    Eigen3::Eigen
    nlohmann_json::nlohmann_json
    ${ROOT_LIBRARIES}
)

add_library(plug INTERFACE)

target_include_directories(plug
  INTERFACE
    $<BUILD_INTERFACE:${RAREXSEC_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(plug
  INTERFACE
    core
    plot
    hist
    data
    utils
)

function(add_plugin NAME SRC EXTRA_LIBS)
  set_source_files_properties(${SRC} PROPERTIES LANGUAGE CXX)
  add_library(${NAME} SHARED ${SRC})
  target_compile_definitions(${NAME} PRIVATE BUILD_PLUGIN)
  target_include_directories(${NAME} PRIVATE
    ${RAREXSEC_INCLUDE_DIR}
  )
  target_link_libraries(${NAME} PRIVATE
    plug
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

add_plugin(RegionsPlugin plug/analytics/RegionsPlugin.cc "")
add_plugin(SnapshotPlugin plug/analytics/SnapshotPlugin.cc "")
add_plugin(VariablesPlugin plug/analytics/VariablesPlugin.cc "")
add_plugin(StrategySelectionPlugin plug/systematics/StrategySelectionPlugin.cc "")

add_plugin(EventDisplayPlugin plug/plotting/EventDisplayPlugin.cc "")
add_plugin(CutFlowPlotPlugin plug/plotting/CutFlowPlotPlugin.cc "")
add_plugin(StackedHistogramPlugin plug/plotting/StackedHistogramPlugin.cc "")
add_plugin(CutMatrixPlotPlugin plug/plotting/CutMatrixPlotPlugin.cc "")
add_plugin(PerformancePlotPlugin plug/plotting/PerformancePlotPlugin.cc "")
add_plugin(SystematicBreakdownPlugin plug/plotting/SystematicBreakdownPlugin.cc "")
add_plugin(UnstackedHistogramPlugin plug/plotting/UnstackedHistogramPlugin.cc "")

add_subdirectory(tests)
add_subdirectory(run)
