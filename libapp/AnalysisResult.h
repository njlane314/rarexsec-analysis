#ifndef ANALYSIS_RESULT_H
#define ANALYSIS_RESULT_H

#include "TFile.h"
#include "TObject.h"

#include "AnalysisTypes.h"
#include "RegionAnalysis.h"

#include <memory>
#include <string>

namespace analysis {

class AnalysisResult : public TObject {
  public:
    AnalysisResult() = default;
    explicit AnalysisResult(RegionAnalysisMap regions) : regions_(std::move(regions)) {}

    const RegionAnalysisMap &regions() const noexcept { return regions_; }
    RegionAnalysisMap &regions() noexcept { return regions_; }

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
};

} // namespace analysis

#endif
