cmake_minimum_required(VERSION 3.10)
project(AnalysisFrameworkProject)

# Find ROOT and include its use file
find_package(ROOT REQUIRED)
include(${ROOT_USE_FILE})

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Check for nlohmann/json include directory
if(NOT DEFINED ENV{NLOHMANN_JSON_INCLUDE_DIR} OR "$ENV{NLOHMANN_JSON_INCLUDE_DIR}" STREQUAL "")
    message(FATAL_ERROR "Environment variable NLOHMANN_JSON_INCLUDE_DIR is not set or is empty. \
This variable should point to the include directory for nlohmann/json.hpp.")
else()
    set(NLOHMANN_JSON_SYSTEM_INCLUDE_PATH $ENV{NLOHMANN_JSON_INCLUDE_DIR})
    message(STATUS "Using nlohmann/json include path from NLOHMANN_JSON_INCLUDE_DIR: ${NLOHMANN_JSON_SYSTEM_INCLUDE_PATH}")
endif()

# Define public headers
set(ANALYSIS_FRAMEWORK_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/SampleLoader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ConfigurationManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/VariableManager.h
)

# Define headers for dictionary generation
set(DICTIONARY_INPUT_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/SampleLoader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ConfigurationManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/VariableManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/HistogramGenerator.h
)

# Generate ROOT dictionary with a unique name
ROOT_GENERATE_DICTIONARY(AnalysisFrameworkDict
    ${DICTIONARY_INPUT_HEADERS}
    LINKDEF ${CMAKE_CURRENT_SOURCE_DIR}/LinkDef.h
    OPTIONS "-I${NLOHMANN_JSON_SYSTEM_INCLUDE_PATH}"
)

# Define the sources for AnalysisFramework
set(ANALYSIS_FRAMEWORK_SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/AnalysisFrameworkDict.cxx
)

# Create the library
add_library(AnalysisFramework ${ANALYSIS_FRAMEWORK_SOURCES})

# Set include directories for the library
target_include_directories(AnalysisFramework
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${ROOT_INCLUDE_DIRS}
        ${NLOHMANN_JSON_SYSTEM_INCLUDE_PATH}
)

# Link the library with ROOT libraries
target_link_libraries(AnalysisFramework PUBLIC ${ROOT_LIBRARIES})

# Install the library, headers, and dictionary PCM file
install(TARGETS AnalysisFramework DESTINATION lib)
install(
    FILES ${ANALYSIS_FRAMEWORK_PUBLIC_HEADERS}
    DESTINATION include/AnalysisFramework
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/AnalysisFrameworkDict_rdict.pcm DESTINATION lib)

# Add executable
add_executable(test_sampleloader test_sampleloader.cpp)

# Set include directories for the executable
target_include_directories(test_sampleloader
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${ROOT_INCLUDE_DIRS}
        ${NLOHMANN_JSON_SYSTEM_INCLUDE_PATH}
)

# Link the executable to the library
target_link_libraries(test_sampleloader
    PRIVATE
        AnalysisFramework
        ${ROOT_LIBRARIES}
)

# Install the executable
install(TARGETS test_sampleloader DESTINATION bin)

add_executable(test_histogram test_histogram.cpp) 

target_include_directories(test_histogram
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${ROOT_INCLUDE_DIRS}
        ${NLOHMANN_JSON_SYSTEM_INCLUDE_PATH}
)

target_link_libraries(test_histogram
    PRIVATE
        AnalysisFramework
        ${ROOT_LIBRARIES}
)

install(TARGETS test_histogram DESTINATION bin)