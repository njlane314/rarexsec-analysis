#ifndef ANALYSIS_RESULT_H
#define ANALYSIS_RESULT_H

#include "TFile.h"
#include "TObject.h"

#include "AnalysisTypes.h"
#include "RegionAnalysis.h"

#include <memory>
#include <string>
#include <map>

namespace analysis {

class AnalysisResult : public TObject {
  public:
    struct VariableResults {
        std::map<VariableKey, VariableResult> variables;
        bool has(const VariableKey &k) const { return variables.find(k) != variables.end(); }
        const VariableResult &get(const VariableKey &k) const { return variables.at(k); }
    };

    AnalysisResult() = default;
    explicit AnalysisResult(RegionAnalysisMap regions) : regions_(std::move(regions)) { build(); }

    const RegionAnalysisMap &regions() const noexcept { return regions_; }
    RegionAnalysisMap &regions() noexcept { return regions_; }

    const RegionAnalysis &region(const RegionKey &r) const { return regions_.at(r); }

    const VariableResult &result(const RegionKey &r, const VariableKey &v) const {
        return variable_results_.at(r).get(v);
    }

    bool hasResult(const RegionKey &r, const VariableKey &v) const {
        auto it = variable_results_.find(r);
        return it != variable_results_.end() && it->second.has(v);
    }

    void build() {
        variable_results_.clear();
        for (auto &rk : regions_) {
            VariableResults vr;
            for (auto &kv : rk.second.finalVariables()) {
                vr.variables.insert_or_assign(kv.first, kv.second);
            }
            variable_results_.insert_or_assign(rk.first, std::move(vr));
        }
    }

    void saveToFile(const std::string &path) const {
        TFile outfile(path.c_str(), "RECREATE");
        outfile.WriteObject(this, "analysis_result");
        outfile.Close();
    }

    static std::unique_ptr<AnalysisResult> loadFromFile(const std::string &path) {
        TFile infile(path.c_str(), "READ");
        auto *ptr = infile.Get<AnalysisResult>("analysis_result");
        return ptr ? std::unique_ptr<AnalysisResult>(static_cast<AnalysisResult *>(ptr->Clone())) : nullptr;
    }

  private:
    RegionAnalysisMap regions_;
    std::map<RegionKey, VariableResults> variable_results_;
};

} // namespace analysis

#endif
