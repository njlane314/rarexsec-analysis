#ifndef DYNAMIC_BINNING_2D_H
#define DYNAMIC_BINNING_2D_H

#include "DynamicBinning.h"

namespace analysis {

class DynamicBinning2D {
  public:
    static std::pair<BinningDefinition, BinningDefinition>
    calculate(std::vector<ROOT::RDF::RNode> nodes, const BinningDefinition &xb, const BinningDefinition &yb,
              const std::string &weight_col = "nominal_event_weight", double min_neff_per_bin = 400.0,
              bool include_out_of_range_bins = false,
              DynamicBinningStrategy strategy = DynamicBinningStrategy::EqualWeight) {
        auto bxnew =
            DynamicBinning::calculate(nodes, xb, weight_col, min_neff_per_bin, include_out_of_range_bins, strategy);

        auto bynew =
            DynamicBinning::calculate(nodes, yb, weight_col, min_neff_per_bin, include_out_of_range_bins, strategy);

        return {bxnew, bynew};
    }
};

} // namespace analysis

#endif
