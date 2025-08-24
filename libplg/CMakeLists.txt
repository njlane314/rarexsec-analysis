add_library(libplg INTERFACE)

target_include_directories(libplg
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(libplg
  INTERFACE
    libapp
    libplot
    libhist
    libdata
    libutils
)

function(add_plugin NAME SRC EXTRA_LIBS)
  add_library(${NAME} SHARED ${SRC})
  target_include_directories(${NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
  )
  target_link_libraries(${NAME} PRIVATE
    libplg
    nlohmann_json::nlohmann_json
    ${ROOT_LIBRARIES}
    ${EXTRA_LIBS}
  )
  set_target_properties(${NAME} PROPERTIES
    PREFIX ""
    POSITION_INDEPENDENT_CODE ON
  )
endfunction()

add_plugin(RegionsPlugin       RegionsPlugin.cpp           "")
add_plugin(VariablesPlugin        VariablesPlugin.cpp         "")
add_plugin(StackedHistogramPlugin StackedHistogramPlugin.cpp  libplot)

foreach(_p IN ITEMS RegionsPlugin VariablesPlugin StackedHistogramPlugin)
  set_target_properties(${_p} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
  )
endforeach()