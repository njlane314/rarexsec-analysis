#ifndef SYSTEMATIC_BREAKDOWN_PLOT_H
#define SYSTEMATIC_BREAKDOWN_PLOT_H

#include <memory>
#include <string>
#include <vector>

#include "TH1D.h"
#include "THStack.h"
#include "TLegend.h"

#include "AnalysisTypes.h"
#include "HistogramPlotterBase.h"

namespace analysis {

class SystematicBreakdownPlot : public HistogramPlotterBase {
  public:
    SystematicBreakdownPlot(std::string plot_name,
                            const VariableResult &var_result,
                            bool normalise = false,
                            std::string output_directory = "plots");

  private:
    void draw(TCanvas &canvas) override;

    std::vector<double> calculateTotals() const;
    void fillHistograms(const std::vector<double> &totals);
    void renderStackLegend();

    const VariableResult &variable_result_;
    bool normalise_;
    std::unique_ptr<THStack> stack_;
    std::vector<std::unique_ptr<TH1D>> histograms_;
    std::unique_ptr<TLegend> legend_;
};

} 

#endif
