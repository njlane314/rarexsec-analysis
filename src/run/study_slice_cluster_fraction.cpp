#include <rarexsec/flow/Study.h>
#include <rarexsec/flow/Where.h>
#include <rarexsec/flow/PlotBuilders.h>
using namespace analysis::dsl;

int main() {
  auto study = Study("Slice Cluster Fraction")
    .data("config/data/samples.json")
    .region("SINGLE_SLICE", where("in_reco_fiducial && (num_slices == 1)"))
    .var("slice_cluster_fraction")
    .plot(stack("slice_cluster_fraction").in("SINGLE_SLICE").signal("inclusive_strange_channels").logY())
    .plot(
      perf("slice_cluster_fraction")
        .in("SINGLE_SLICE")
        .channel("incl_channel")
        .signal("inclusive_strange_channels")
        .bins(50, 0.0, 1.0)
        .cut(dir::gt)
        .name("slice_cluster_fraction_perf")
        .out("plots/perf")
    );

  study.run("/tmp/slice_cluster_fraction.root");
  return 0;
}
