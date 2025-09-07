#ifndef REGION_HANDLE_H
#define REGION_HANDLE_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "AnalysisKey.h"
#include "RegionAnalysis.h"
#include "Selection.h"

namespace analysis {

class RegionHandle {
  public:
    RegionHandle(const RegionKey &k, const std::map<RegionKey, std::string> &names,
                 const std::map<RegionKey, Selection> &sels,
                 const std::map<RegionKey, std::unique_ptr<RegionAnalysis>> &analyses,
                 const std::map<RegionKey, std::vector<VariableKey>> &vars)
        : key_(k), names_(names), selections_(sels), analyses_(analyses), variables_(vars) {}

    const std::string &name() const { return names_.at(key_); }
    const Selection &selection() const { return selections_.at(key_); }

    std::unique_ptr<RegionAnalysis> &analysis() const {
        return const_cast<std::unique_ptr<RegionAnalysis> &>(analyses_.at(key_));
    }

    const std::vector<VariableKey> &vars() const {
        auto it = variables_.find(key_);
        static const std::vector<VariableKey> empty;
        return it != variables_.end() ? it->second : empty;
    }

    const RegionKey key_;

  private:
    const std::map<RegionKey, std::string> &names_;
    const std::map<RegionKey, Selection> &selections_;
    const std::map<RegionKey, std::unique_ptr<RegionAnalysis>> &analyses_;
    const std::map<RegionKey, std::vector<VariableKey>> &variables_;
};

}

#endif
