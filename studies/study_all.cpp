#include <workflow/Study.h>
#include <workflow/Helpers.h>
#include <workflow/Display.h>
#include <workflow/Plots.h>
#include <workflow/Snapshot.h>
using namespace analysis::workflow;

int main() {
  auto s = Study("NUMU CC end-to-end")
    .data("config/samples.json")
    .region("NUMU_CC", where("QUALITY && NUMU_CC"))
    .var("topological_score")
    .plot(
      perf("topological_score")
        .in("NUMU_CC")
        .channel("incl_channel")
        .signal("inclusive_strange_channels")
        .bins(100, 0.0, 1.0)
        .cut(dir::gt)
        .name("topo_perf")
        .out("plots/perf")
        .where_all({"in_reco_fiducial", "num_slices==1"})
    )
    .plot(
      cutflow()
        .in("NUMU_CC")
        .rule("NUMU_CC")
        .channel("incl_channel")
        .signal("inclusive_strange_channels")
        .initial("All events")
        .steps({"QUALITY", "VTX", "TOPO", "PID"})
        .name("numu_cc_cutflow")
        .logY()
        .out("plots/cutflow")
    )
    .display(
      events().from("numi_on").in("NUMU_CC")
        .limit(12).size(900)
        .planes({"U","V","W"})
        .mode(overlay().alpha(0.35))
        .out("plots/event_displays")
    )
    .snapshot(
      snapshot()
        .rule("NUMU_CC")
        .out("snapshots")
        .columns({"run","sub","evt","topological_score"})
    );

  s.run("/tmp/out.root");
  return 0;
}
