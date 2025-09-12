#include <rarexsec/flow/Study.h>
#include <rarexsec/flow/Where.h>
#include <rarexsec/flow/EventDisplayBuilder.h>
using namespace analysis::dsl;

int main() {
  auto study = Study("Region detector and semantic displays")
    .data("config/catalogs/samples.json")
    // Use concrete selection expression instead of symbolic rule names
    // to avoid ROOT JIT errors from undefined identifiers.
    .region("NUMU_CC", where("quality_event && has_muon"))
    .display(
      events().from("numi_on").in("NUMU_CC")
        .limit(5).size(800)
        .mode(detector())
        .out("plots/event_displays/detector")
    )
    .display(
      events().from("numi_on").in("NUMU_CC")
        .limit(5).size(800)
        .mode(semantic())
        .out("plots/event_displays/semantic")
    );

  study.run("/tmp/event_displays.root");
  return 0;
}
