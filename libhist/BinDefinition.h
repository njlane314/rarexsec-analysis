#ifndef BIN_DEFINITION_H
#define BIN_DEFINITION_H

#include <vector>
#include <string>
#include <algorithm>
#include "Logger.h"
#include "TypeKey.h"

namespace analysis {

using BranchExpression = std::string;
using TexAxisLabel = std::string;

class BinDefinition {
public:
    std::vector<double>         edges_;
    BranchExpression            branch_;
    TexAxisLabel                tex_;
    std::vector<SelectionKey>   selec_keys_;
    StratifierKey               strat_key_;

    BinDefinition(std::vector<double> ed,
                  const std::string& br,
                  const std::string& tx,
                  std::vector<std::string> ks,
                  const std::string& sk = "")
      : edges_(std::move(ed))
      , branch_(br) 
      , tex_(tx)
      , selec_keys_(std::move(ks))
      , strat_key_(StratifierKey{sk})
    {
        if (edges_.size() < 2)
            log::fatal("BinDefinition", "Edges must contain at least two values.");

        if (!std::is_sorted(edges_.begin(), edges_.end()))
            log::fatal("BinDefinition", "Edges must be sorted.");
    }

    BinDefinition() = default;

    const BranchExpression& getVariable() const { return branch_; }
    const TexAxisLabel& getTexLabel() const { return tex_; }
    const std::vector<SelectionKey>& getSelectionKeys() const { return selec_keys_; }
    const StratifierKey& getStratifierKey() const { return strat_key_; }

    void setVariable(BranchExpression var) { branch_ = std::move(var); }
    void setStratifierKey(StratifierKey sk) { strat_key_ = std::move(sk); }
};

}

#endif