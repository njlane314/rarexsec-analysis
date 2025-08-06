#ifndef BIN_DEFINITION_H
#define BIN_DEFINITION_H

#include <vector>
#include <string>
#include <algorithm>
#include "TString.h"
#include "Logger.h"

namespace analysis {

class BinDefinition {
public:
    std::vector<double>  edges_;
    TString              branch_;
    TString              name_;
    TString              tex_;
    std::vector<TString> keys_;
    std::string          stratification_key;


    BinDefinition(std::vector<double> ed,
                  const std::string&  br,
                  const std::string&  tx,
                  std::vector<TString> ks,
                  const std::string&  nm,
                  const std::string& sk = "")
      : edges_(std::move(ed))
      , branch_(br.c_str())
      , name_(nm.c_str())
      , tex_(tx.c_str())
      , keys_(std::move(ks))
      , stratification_key(sk)
    {
        if (edges_.size() < 2)
            log::fatal("BinDefinition", "Edges must contain at least two values.");

        if (!std::is_sorted(edges_.begin(), edges_.end()))
            log::fatal("BinDefinition", "Edges must be sorted.");
    }

    BinDefinition() = default;

    std::size_t nBins() const noexcept {
        return edges_.size() > 1 ? edges_.size() - 1 : 0;
    }

    const TString& getVariable() const { return branch_; }
    const TString& getName() const { return name_; }
    const TString& getTexLabel() const { return tex_; }
    const std::vector<TString>& getSelectionKeys() const { return keys_; }
    void setVariable(TString var) { branch_ = var; }
};

}

#endif