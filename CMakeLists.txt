cmake_minimum_required(VERSION 3.10)
project(AnalysisFrameworkProject)

# Find ROOT and include its use file
find_package(ROOT REQUIRED)
include(${ROOT_USE_FILE})

find_package(TBB COMPONENTS tbb tbbmalloc REQUIRED)
message(STATUS "Found TBB: ${TBB_FOUND}")
if(TBB_FOUND)
    message(STATUS "TBB include_dirs: ${TBB_INCLUDE_DIRS}") # For debugging
    message(STATUS "TBB libraries: ${TBB_LIBRARIES}")     # For debugging
    if(TARGET TBB::tbb)
        message(STATUS "SUCCESS: IMPORTED target TBB::tbb IS NOW DEFINED.")
    else()
        message(WARNING "TBB::tbb target STILL NOT DEFINED after requesting components.")
    endif()
    if(TARGET TBB::tbbmalloc) # Check if tbbmalloc target is also defined
         message(STATUS "Found IMPORTED target TBB::tbbmalloc.")
    endif()
endif()
find_package(Torch REQUIRED)
message(STATUS "Found LibTorch version: ${TORCH_VERSION}")

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
    ${CMAKE_CURRENT_SOURCE_DIR}/DataManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AnalysisRunner.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AnalysisSpace.h
)

# Define headers for dictionary generation
set(DICTIONARY_INPUT_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/DataManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AnalysisRunner.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AnalysisSpace.h
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




function(add_analysis_executable target_name source_file)
    add_executable(${target_name} ${source_file})

    target_include_directories(${target_name}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include # Access to AnalysisFramework headers
            ${ROOT_INCLUDE_DIRS}
            ${NLOHMANN_JSON_SYSTEM_INCLUDE_PATH} # Or link against nlohmann_json::nlohmann_json
            ${TORCH_INCLUDE_DIRS}
    )

    target_link_libraries(${target_name}
        PRIVATE
            AnalysisFramework
            ${ROOT_LIBRARIES}
            ${TORCH_LIBRARIES}
            # If nlohmann/json is an imported target:
            # nlohmann_json::nlohmann_json
    )

    install(TARGETS ${target_name} DESTINATION bin)
endfunction()


