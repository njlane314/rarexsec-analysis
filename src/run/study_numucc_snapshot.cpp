#include <rarexsec/flow/SnapshotBuilder.h>
#include <rarexsec/flow/Study.h>
#include <rarexsec/flow/Where.h>

using namespace analysis::dsl;

int main() {
  auto study =
      Study("NuMu CC snapshot")
          .data("config/catalogs/samples.json")
          .region("NUMU_CC", where("QUALITY && NUMU_CC"))
          .var("run")
          .var("sub")
          .var("evt")
          .var("reco_neutrino_energy")
          .var("topological_score")
          .snapshot(snapshot()
                        .rule("NUMU_CC")
                        .out("snapshots")
                        .columns({"run", "sub", "evt", "reco_neutrino_energy",
                                  "topological_score"}));

  study.run("/tmp/numu_cc_snapshot.root");
  return 0;
}
