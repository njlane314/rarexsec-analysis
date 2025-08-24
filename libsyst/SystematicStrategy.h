#ifndef SYSTEMATIC_STRATEGY_H
#define SYSTEMATIC_STRATEGY_H

#include <functional>
#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <TMatrixDSym.h>
#include "BinningDefinition.h"
#include "BinnedHistogram.h"
#include "ROOT/RDataFrame.hxx"
#include "TH1D.h"
#include "SampleTypes.h"
#include "AnalysisTypes.h"
#include "Keys.h"

namespace analysis {

struct SystematicFutures {
    std::unordered_map<
        SystematicKey,
        std::map<
            SampleKey,
            ROOT::RDF::RResultPtr<TH1D>
        >
    > variations;
};

// This is no longer needed: using BookHistFn = std::function<ROOT::RDF::RResultPtr<TH1D>(const std::string&)>;

struct UniverseDef {
    std::string name_;
    std::string vector_name_;
    unsigned    n_universes_;
};

struct KnobDef {
    std::string name_;
    std::string up_column_;
    std::string dn_column_;
};

class SystematicStrategy {
public:
    virtual ~SystematicStrategy() = default;

    virtual const std::string&
    getName() const = 0;

    virtual void
    bookVariations(
        const SampleKey&        sample_key,
        ROOT::RDF::RNode&       rnode,
        const BinningDefinition& binning,
        const ROOT::RDF::TH1DModel& model,
        SystematicFutures&      futures
    ) = 0;

    virtual TMatrixDSym
    computeCovariance(
        VariableResult&  result,
        SystematicFutures&     futures
    ) = 0;

    virtual std::map<SystematicKey, BinnedHistogram>
    getVariedHistograms(
        const BinningDefinition& bin,
        SystematicFutures& futures
    ) = 0;
};

}

#endif