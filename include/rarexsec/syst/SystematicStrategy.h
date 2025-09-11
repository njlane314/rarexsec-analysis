#ifndef SYSTEMATIC_STRATEGY_H
#define SYSTEMATIC_STRATEGY_H


#include <functional>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "ROOT/RDataFrame.hxx"
#include "TH1D.h"
#include <TMatrixDSym.h>

#include <rarexsec/core/VariableResult.h>
#include <rarexsec/hist/BinnedHistogram.h>
#include <rarexsec/hist/BinningDefinition.h>
#include <rarexsec/core/AnalysisKey.h>
#include <rarexsec/data/SampleTypes.h>

namespace analysis {

using VariationFutures =
    std::unordered_map<SystematicKey, std::map<SampleKey, ROOT::RDF::RResultPtr<TH1D>>>;

struct SystematicFutures {
    VariationFutures variations;
};

struct UniverseDef {
    std::string name_;
    std::string vector_name_;
    unsigned n_universes_{};
};

struct KnobDef {
    std::string name_;
    std::string up_column_;
    std::string dn_column_;
};

class SystematicStrategy {
  public:
    virtual ~SystematicStrategy() = default;

    virtual const std::string &getName() const = 0;

    virtual void bookVariations(const SampleKey &sample_key, ROOT::RDF::RNode &rnode, const BinningDefinition &binning,
                                const ROOT::RDF::TH1DModel &model, SystematicFutures &futures) = 0;

    virtual TMatrixDSym computeCovariance(VariableResult &result, SystematicFutures &futures) = 0;

    virtual std::map<SystematicKey, BinnedHistogram> getVariedHistograms(const BinningDefinition &bin,
                                                                         SystematicFutures &futures) = 0;
};

}

#endif
