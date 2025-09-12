#include <rarexsec/flow/PlotBuilders.h>
#include <rarexsec/flow/Study.h>
#include <rarexsec/flow/Where.h>

using namespace analysis::dsl;

int main() {
  auto s = Study("NuMu CC signal survival")
               .data("config/catalogs/samples.json")
               .region("NUMU_CC", where("quality_event && has_muon"))
               .plot(survival()
                         .name("numu_cc_signal_survival")
                         .out("plots/survival"));

  s.run("/tmp/numu_cc_signal_survival.root");
  return 0;
}
