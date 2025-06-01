set(FRAMEWORK_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/AnalysisRunner.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AnalysisSpace.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Binning.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ConfigurationManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/DataManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/DataTypes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/DefinitionManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/EventCategories.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Histogram.h
    ${CMAKE_CURRENT_SOURCE_DIR}/HistogramGenerator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Selection.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Systematics.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SystematicsController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/VariableManager.h
)

set(DICTIONARY_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/DataManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AnalysisRunner.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AnalysisSpace.h
)

message(STATUS "framework/CMakeLists.txt now sees JSON path as: ${NLOHMANN_JSON_INCLUDE_DIR}")

ROOT_GENERATE_DICTIONARY(AnalysisFrameworkDict
    ${DICTIONARY_HEADERS}
    LINKDEF ${CMAKE_CURRENT_SOURCE_DIR}/LinkDef.h
    OPTIONS "-I${NLOHMANN_JSON_INCLUDE_DIR}"
)

add_library(AnalysisFramework "")

target_sources(AnalysisFramework
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/AnalysisFrameworkDict.cxx
)

target_include_directories(AnalysisFramework
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${NLOHMANN_JSON_INCLUDE_DIR}
)