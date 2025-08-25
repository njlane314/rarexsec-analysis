#ifndef BINNING_DEFINITION_H
#define BINNING_DEFINITION_H

#include <ROOT/RDataFrame.hxx>
#include <algorithm>
#include <string>
#include <vector>

#include "AnalysisLogger.h"
#include "KeyTypes.h"

namespace analysis {

using BranchExpression = std::string;
using TexAxisLabel = std::string;

class BinningDefinition {
  public:
    BinningDefinition(std::vector<double> ed, const std::string &br,
                      const std::string &tx, std::vector<SelectionKey> ks,
                      const std::string &sk = "")
        : edges_(std::move(ed)), branch_(br), tex_(tx),
          selec_keys_(std::move(ks)), strat_key_(StratifierKey{sk}) {
        if (edges_.size() < 2)
            log::fatal("BinningDefinition::BinningDefinition",
                       "Edges must contain at least two values.");

        if (!std::is_sorted(edges_.begin(), edges_.end()))
            log::fatal("BinningDefinition::BinningDefinition",
                       "Edges must be sorted.");
    }

    BinningDefinition() = default;

    const std::vector<double> &getEdges() const { return edges_; }
    const BranchExpression &getVariable() const { return branch_; }
    const TexAxisLabel &getTexLabel() const { return tex_; }
    const std::vector<SelectionKey> &getSelectionKeys() const {
        return selec_keys_;
    }
    const StratifierKey &getStratifierKey() const { return strat_key_; }
    size_t getBinNumber() const { return edges_.size() - 1; }

    ROOT::RDF::TH1DModel toTH1DModel() const {
        return ROOT::RDF::TH1DModel(getVariable().c_str(),
                                    getTexLabel().c_str(), getBinNumber(),
                                    getEdges().data());
    }

  private:
    std::vector<double> edges_;
    BranchExpression branch_;
    TexAxisLabel tex_;
    std::vector<SelectionKey> selec_keys_;
    StratifierKey strat_key_;
};

} // namespace analysis

#endif