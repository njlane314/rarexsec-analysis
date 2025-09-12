#include <rarexsec/flow/Study.h>
#include <rarexsec/flow/Where.h>
#include <rarexsec/flow/PlotBuilders.h>
using namespace analysis::dsl;

int main() {
  auto study = Study("MC Neutrino Energy")
    .data("config/catalogs/samples.json")
    .mcOnly()
    .region("EMPTY", where(""))
    // The WeightProcessor automatically scales MC event weights to the
    // total protons-on-target, so the resulting histogram is POT-normalised.
    .var(VarDef("neutrino_energy")
             .bins(100, 0.0, 10.0)
             .stratify("channel_definitions"))
    .plot(stack("neutrino_energy")
              .in("EMPTY")
              .signal("channel_definitions")
              .channel("channel_definitions"));

  study.run("/tmp/neutrino_energy.root");
  return 0;
}
