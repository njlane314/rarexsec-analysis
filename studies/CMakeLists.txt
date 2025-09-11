add_executable(study_topo_score
  study_topo_score.cpp
  presets/Presets_Standard.cpp
  presets/Presets_Vertices.cpp
  presets/Presets_Plots.cpp
  presets/Presets_Preselections.cpp
  presets/Presets_TopologicalScore.cpp)

add_executable(study_all
  study_all.cpp
  presets/Presets_Standard.cpp
  presets/Presets_Vertices.cpp
  presets/Presets_Plots.cpp
  presets/Presets_Preselections.cpp
  presets/Presets_TopologicalScore.cpp)

target_link_libraries(study_topo_score PRIVATE
  libapp
  libplug
  libdata
  libhist
  libplot
  libsyst
  libutils
  Eigen3::Eigen
  nlohmann_json::nlohmann_json
  ${ROOT_LIBRARIES}
  TBB::tbb
  TMVA
  dl)

target_link_libraries(study_all PRIVATE
  libapp
  libplug
  libdata
  libhist
  libplot
  libsyst
  libutils
  Eigen3::Eigen
  nlohmann_json::nlohmann_json
  ${ROOT_LIBRARIES}
  TBB::tbb
  TMVA
  dl)
