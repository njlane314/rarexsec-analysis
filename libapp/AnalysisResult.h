#ifndef ANALYSIS_RESULT_H
#define ANALYSIS_RESULT_H

#include "TFile.h"
#include "TObject.h"

#include "VariableResult.h"
#include "RegionAnalysis.h"

#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <string>

namespace analysis {

class AnalysisResult : public TObject {
  public:
    struct VariableResults {
        std::map<VariableKey, VariableResult> variables;
        bool has(const VariableKey &k) const { return variables.find(k) != variables.end(); }
        const VariableResult &get(const VariableKey &k) const { return variables.at(k); }
    };

    AnalysisResult() = default;
    explicit AnalysisResult(RegionAnalysisMap regions) : regions_(std::move(regions)) { this->build(); }

    const RegionAnalysisMap &regions() const noexcept { return regions_; }
    RegionAnalysisMap &regions() noexcept { return regions_; }

    const RegionAnalysis &region(const RegionKey &r) const { return regions_.at(r); }
    const std::vector<RegionAnalysis::StageCount> &cutFlow(const RegionKey &r) const {
        return regions_.at(r).cutFlow();
    }

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

    std::map<std::string, AnalysisResult> resultsByBeam() const {
        std::map<std::string, AnalysisResult> m;

        for (auto const &kv : regions_)
            m[kv.second.beamConfig()].regions().insert(kv);

        for (auto &[k, v] : m)
            v.build();

        return m;
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

    static void printSummary(const VariableResult &r) {
        auto name = r.binning_.getVariable();
        auto bins = r.binning_.getBinNumber();

        size_t width = 70;
        std::string line(width, '=');
        std::string sub(width, '-');

        std::cout << '\n' << line << '\n';
        std::cout << std::left << std::setw(width) << ("Variable: " + name) << '\n';
        std::cout << line << '\n';

        std::cout << std::fixed << std::setprecision(2);
        std::cout << std::left << std::setw(30) << "Bins" << std::right << std::setw(width - 30) << bins << '\n';
        std::cout << std::left << std::setw(30) << "Total Data Events" << std::right << std::setw(width - 30)
                  << r.data_hist_.getSum() << '\n';
        std::cout << std::left << std::setw(30) << "Total MC Events" << std::right << std::setw(width - 30)
                  << r.total_mc_hist_.getSum() << '\n';

        std::cout << sub << '\n';
        std::cout << std::left << std::setw(width) << "Stratum MC Sums" << '\n';
        for (const auto &[k, h] : r.strat_hists_)
            std::cout << std::left << std::setw(30) << k.str() << std::right << std::setw(width - 30) << h.getSum()
                      << '\n';

        if (!r.covariance_matrices_.empty()) {
            std::cout << sub << '\n';
            std::cout << std::left << std::setw(width) << "Available Systematics" << '\n';
            for (const auto &[k, c] : r.covariance_matrices_)
                if (c.GetNrows() > 0)
                    std::cout << k.str() << '\n';
        }

        std::cout << line << "\n\n";
    }

  private:
    RegionAnalysisMap regions_;
    std::map<RegionKey, VariableResults> variable_results_;
};

}

#endif
