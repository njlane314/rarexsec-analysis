set(FRAMEWORK_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/AnalysisChannels.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AnalysisResult.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AnalysisRunner.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AnalysisSpace.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Binning.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ConfigurationManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/DataManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/DataTypes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/DefinitionManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/EventDisplay.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Histogram.h
    ${CMAKE_CURRENT_SOURCE_DIR}/HistogramGenerator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PlotBase.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PlotManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PlotStacked.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PlotSysCorrelation.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PlotVarCorrelation.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Selection.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Systematics.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SystematicsController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/VariableManager.h
)

message(STATUS "framework/CMakeLists.txt now sees JSON path as: ${NLOHMANN_JSON_INCLUDE_DIR}")

ROOT_GENERATE_DICTIONARY(AnalysisFrameworkDict
    ${FRAMEWORK_HEADERS}
    LINKDEF ${CMAKE_CURRENT_SOURCE_DIR}/LinkDef.h
    OPTIONS "-I${NLOHMANN_JSON_INCLUDE_DIR}"
)

add_library(AnalysisFramework SHARED ${CMAKE_CURRENT_BINARY_DIR}/AnalysisFrameworkDict.cxx)

add_dependencies(AnalysisFramework AnalysisFrameworkDict)

target_include_directories(AnalysisFramework
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${NLOHMANN_JSON_INCLUDE_DIR}
)