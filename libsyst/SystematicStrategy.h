#ifndef SYSTEMATIC_STRATEGY_H
#define SYSTEMATIC_STRATEGY_H

#include <functional>
#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <TMatrixDSym.h>
#include "BinDefinition.h"
#include "ROOT/RDataFrame.hxx"
#include "TH1D.h"

namespace analysis {

class BinnedHistogram;

struct SystematicFutures {
    std::unordered_map<int, ROOT::RDF::RResultPtr<TH1D>> nominal;
    std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<int, ROOT::RDF::RResultPtr<TH1D>>>> variations;
};

using BookHistFn = std::function<ROOT::RDF::RResultPtr<TH1D>(int, const std::string&)>;

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
        const std::vector<int>& keys,
        BookHistFn              book_fn,
        SystematicFutures&      futures
    ) = 0;

    virtual TMatrixDSym
    computeCovariance(
        int                    key,
        const BinnedHistogram& nominal_hist,
        SystematicFutures&     futures
    ) = 0;

    virtual std::map<std::string, BinnedHistogram>
    getVariedHistograms(
        int key,
        const BinDefinition& bin,
        SystematicFutures& futures
    ) = 0;
};

}

#endif