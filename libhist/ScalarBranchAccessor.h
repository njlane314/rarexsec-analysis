#ifndef SCALAR_BRANCH_ACCESSOR_H
#define SCALAR_BRANCH_ACCESSOR_H

#include "IBranchAccessor.h"
#include <ROOT/RDataFrame.hxx>
#include <vector>
#include <string>

namespace analysis {

class ScalarBranchAccessor : public IBranchAccessor {
public:
    std::vector<double> extractValues(
        ROOT::RDF::RNode  df,
        const std::string& expr
    ) const override
    {
        auto resultPtr = df.Take<double>(expr);
        const auto& vals = *resultPtr;
        return std::vector<double>(vals.begin(), vals.end());
    }
};

}

#endif // SCALAR_BRANCH_ACCESSOR_H