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

#include "VariableResult.h"
#include "BinnedHistogram.h"
#include "BinningDefinition.h"
#include "KeyTypes.h"
#include "SampleTypes.h"

namespace analysis {

struct SystematicFutures {
    std::unordered_map<SystematicKey, std::map<SampleKey, ROOT::RDF::RResultPtr<TH1D>>> variations;
};

struct UniverseDef {
    std::string name_;
    std::string vector_name_;
    unsigned n_universes_;
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