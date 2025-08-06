#ifndef IBRANCH_ACCESSOR_H
#define IBRANCH_ACCESSOR_H

#include <vector>
#include <string>
#include "ROOT/RDataFrame.hxx"

namespace analysis {

enum class BranchType { Scalar, Vector };

class IBranchAccessor {
public:
    virtual ~IBranchAccessor() = default;

    virtual std::vector<double> extractValues(
        ROOT::RDF::RNode     df,
        const std::string&   expr
    ) const = 0;
};

}

#endif // IBRANCH_ACCESSOR_H
