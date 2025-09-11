#include <rarexsec/flow/Study.h>
#include <rarexsec/flow/Where.h>
#include <rarexsec/flow/PlotBuilders.h>
using namespace analysis::dsl;

int main() {
  auto study = Study("Reconstructed Neutrino Vertex")
    .data("config/data/samples.json")
    .region("QUALITY", where("quality_event"))
    .var("reco_neutrino_vertex_x")
    .var("reco_neutrino_vertex_y")
    .var("reco_neutrino_vertex_z")
    .plot(stack("reco_neutrino_vertex_x").in("QUALITY").signal("inclusive_strange_channels").logY())
    .plot(stack("reco_neutrino_vertex_y").in("QUALITY").signal("inclusive_strange_channels").logY())
    .plot(stack("reco_neutrino_vertex_z").in("QUALITY").signal("inclusive_strange_channels").logY());

  study.run("/tmp/neutrino_vertex.root");
  return 0;
}
