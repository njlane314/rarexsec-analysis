#ifndef VARIABLE_HANDLE_H
#define VARIABLE_HANDLE_H

#include <map>
#include <string>

#include <rarexsec/hist/BinningDefinition.h>
#include <rarexsec/app/AnalysisKey.h>

namespace analysis {

class VariableHandle {
  public:
    VariableHandle(const VariableKey &k, const std::map<VariableKey, std::string> &exprs,
                   const std::map<VariableKey, std::string> &lbls,
                   const std::map<VariableKey, BinningDefinition> &bdefs,
                   const std::map<VariableKey, std::string> &strats)
        : key_(k), expressions_(exprs), labels_(lbls), binnings_(bdefs), stratifiers_(strats) {}

    const std::string &expression() const { return expressions_.at(key_); }
    const std::string &label() const { return labels_.at(key_); }
    const BinningDefinition &binning() const { return binnings_.at(key_); }

    std::string stratifier() const {
        auto it = stratifiers_.find(key_);
        return it != stratifiers_.end() ? it->second : "";
    }

    const VariableKey key_;

  private:
    const std::map<VariableKey, std::string> &expressions_;
    const std::map<VariableKey, std::string> &labels_;
    const std::map<VariableKey, BinningDefinition> &binnings_;
    const std::map<VariableKey, std::string> &stratifiers_;
};

}

#endif
