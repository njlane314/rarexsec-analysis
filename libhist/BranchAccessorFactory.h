#ifndef BRANCH_ACCESSOR_FACTORY_H
#define BRANCH_ACCESSOR_FACTORY_H

#include "IBranchAccessor.h"
#include "ScalarBranchAccessor.h"
#include "VectorBranchAccessor.h"

#include <unordered_map>
#include <functional>

namespace analysis {

class BranchAccessorFactory {
public:
    using acc_creator = std::function<std::unique_ptr<IBranchAccessor>()>;

    static std::unique_ptr<IBranchAccessor> create(BranchType type) {
        auto it = creators_.find(type);
        if (it == creators_.end()) {
            log::fatal("BranchAccessorFactory", "Unknown BranchType");
        }
        return it->second();
    }

private:
    static const std::unordered_map<BranchType, acc_creator> creators_;
};

const std::unordered_map<BranchType, BranchAccessorFactory::acc_creator>
BranchAccessorFactory::creators_ = {
    { BranchType::Scalar, []() { return std::make_unique<ScalarBranchAccessor>(); } },
    { BranchType::Vector, []() { return std::make_unique<VectorBranchAccessor>(); } }
};

}

#endif // BRANCH_ACCESSOR_FACTORY_H
