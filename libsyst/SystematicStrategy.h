#ifndef SYSTEMATIC_STRATEGY_H
#define SYSTEMATIC_STRATEGY_H

#include <functional>
#include <map>
#include <string>
#include <vector>
#include <TMatrixDSym.h>
#include "BinDefinition.h"

namespace analysis {

class BinnedHistogram;

using BookHistFn = std::function<BinnedHistogram(int, const std::string&)>;

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
        BookHistFn              book_fn
    ) = 0;

    virtual TMatrixDSym
    computeCovariance(
        int              key,
        const BinnedHistogram& nominal_hist
    ) = 0;

    virtual std::map<std::string, BinnedHistogram>
    getVariedHistograms(
        int key
    ) = 0;
};

}

#endif




