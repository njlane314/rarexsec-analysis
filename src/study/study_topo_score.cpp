#include <rarexsec/study/Study.h>
#include <rarexsec/study/Display.h>
#include <rarexsec/study/Helpers.h>
#include <rarexsec/study/Plots.h>
using namespace analysis::dsl;

int main() {
  auto study = Study("Topo score")
    .data("config/data/samples.json")
    .region("PRE_TOPO", where("in_reco_fiducial && (num_slices == 1) && (optical_filter_pe_beam > 20)"))
    .var("topological_score")
    .plot(stack("topological_score").in("PRE_TOPO").signal("inclusive_strange_channels").logY())
    .plot(roc("topological_score").in("PRE_TOPO").channel("incl_channel").signal("inclusive_strange_channels"))
    .display(
      events().from("numi_on").in("PRE_TOPO")
        .limit(12).size(800).planes({"U","V","W"})
        .mode(detector())
        .out("plots/event_displays")
        .name("{plane}_{run}_{sub}_{evt}")
    );

  study.run("/tmp/output.root");
  return 0;
}
