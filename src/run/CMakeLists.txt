add_executable(study_topo_score
  study_topo_score.cpp)

add_executable(study_all
  study_all.cpp)

add_executable(study_slice_cluster_fraction
  study_slice_cluster_fraction.cpp)

add_executable(study_neutrino_vertex
  study_neutrino_vertex.cpp)

target_link_libraries(study_topo_score PRIVATE
  core
  plug
  data
  hist
  plot
  syst
  utils
  Eigen3::Eigen
  nlohmann_json::nlohmann_json
  ${ROOT_LIBRARIES}
  TBB::tbb
  TMVA
  dl)

target_link_libraries(study_all PRIVATE
  core
  plug
  data
  hist
  plot
  syst
  utils
  Eigen3::Eigen
  nlohmann_json::nlohmann_json
  ${ROOT_LIBRARIES}
  TBB::tbb
  TMVA
  dl)

target_link_libraries(study_slice_cluster_fraction PRIVATE
  core
  plug
  data
  hist
  plot
  syst
  utils
  Eigen3::Eigen
  nlohmann_json::nlohmann_json
  ${ROOT_LIBRARIES}
  TBB::tbb
  TMVA
  dl)

target_link_libraries(study_neutrino_vertex PRIVATE
  core
  plug
  data
  hist
  plot
  syst
  utils
  Eigen3::Eigen
  nlohmann_json::nlohmann_json
  ${ROOT_LIBRARIES}
  TBB::tbb
  TMVA
  dl)

set_target_properties(study_topo_score study_all study_slice_cluster_fraction study_neutrino_vertex PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
