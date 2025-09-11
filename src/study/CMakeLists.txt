add_executable(study_topo_score
  study_topo_score.cpp)

add_executable(study_all
  study_all.cpp)

target_link_libraries(study_topo_score PRIVATE
  libcore
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
  libcore
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
