#ifndef VECTOR_BRANCH_ACCESSOR_H
#define VECTOR_BRANCH_ACCESSOR_H

#include "IBranchAccessor.h"
#include "ROOT/RDataFrame.hxx"
#include "ROOT/RVec.hxx"
#include <vector>
#include <string>

namespace analysis {

class VectorBranchAccessor : public IBranchAccessor {
public:
    std::vector<double> extractValues(
        ROOT::RDF::RNode df,
        const std::string& expr) const override
    {
        auto fut = df.Take<ROOT::RVec<float>>(expr);
        const auto& vecs = *fut.GetResult();
        std::vector<double> all;
        for (auto const& v : vecs)
            all.insert(all.end(), v.begin(), v.end());
        return all;
    }
};

} 

#endif // VECTOR_BRANCH_ACCESSOR_H
