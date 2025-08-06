#ifndef VECTOR_BRANCH_ACCESSOR_H
#define VECTOR_BRANCH_ACCESSOR_H

#include "IBranchAccessor.h"
#include "ROOT/RDataFrame.hxx"
#include "ROOT/RVec.hxx"
#include "Logger.h"
#include <vector>
#include <string>

namespace analysis {

class VectorBranchAccessor : public IBranchAccessor {
    std::vector<double> unpackInts(ROOT::RDF::RNode df, const std::string& expr) const {
        auto fut = df.Take<ROOT::VecOps::RVec<int>>(expr);
        const auto& vecs = *fut;
        std::vector<double> out;
        for (auto const& v : vecs)
            for (int x : v)
                out.push_back(double(x));
        return out;
    }

    std::vector<double> unpackFloats(ROOT::RDF::RNode df, const std::string& expr) const {
        auto fut = df.Take<ROOT::VecOps::RVec<float>>(expr);
        const auto& vecs = *fut;
        std::vector<double> out;
        for (auto const& v : vecs)
            for (float x : v)
                out.push_back(double(x));
        return out;
    }

public:
    std::vector<double> extractValues(
        ROOT::RDF::RNode df,
        const std::string& expr
    ) const override
    {
        auto colType = df.GetColumnType(expr);
        log::info("VectorBranchAccessor",
                  "extractValues called for expr=", expr,
                  " detected column type=", colType);

        if (colType == "ROOT::VecOps::RVec<int>" ||
            colType == "ROOT::RVec<int>" ||
            colType == "std::vector<int>") {
            return unpackInts(df, expr);
        }
        if (colType == "ROOT::VecOps::RVec<float>" ||
            colType == "ROOT::RVec<float>" ||
            colType == "std::vector<float>") {
            return unpackFloats(df, expr);
        }

        log::fatal("VectorBranchAccessor",
                   "Unsupported vector column type=", colType,
                   " for expr=", expr);
        return {};
    }
};

}

#endif // VECTOR_BRANCH_ACCESSOR_H
