#include <rarexsec/flow/PlotBuilders.h>
#include <rarexsec/flow/Study.h>
#include <rarexsec/flow/Where.h>

using namespace analysis::dsl;

int main() {
  auto s = Study("NuMu CC selection and purity")
               .data("config/catalogs/samples.json")
               .region("NUMU_CC", where("QUALITY && NUMU_CC"))
               .plot(cutflow()
                         .rule("NUMU_CC")
                         .in("NUMU_CC")
                         .channel("incl_channel")
                         .signal("inclusive_strange_channels")
                         .initial("All events")
                         .steps({"QUALITY", "VTX", "TOPO", "PID"})
                         .name("numu_cc_selection")
                         .out("plots/selection"));

  s.run("/tmp/numu_cc_selection.root");
  return 0;
}
