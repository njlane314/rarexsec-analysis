#ifndef SYSTEMATIC_BREAKDOWN_PLOT_H
#define SYSTEMATIC_BREAKDOWN_PLOT_H

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
    ~SystematicBreakdownPlot() override;

  private:
    void draw(TCanvas &canvas) override;

    const VariableResult &variable_result_;
    bool normalise_;
    THStack *stack_;
    std::vector<TH1D *> histograms_;
    TLegend *legend_;
};

} // namespace analysis

#endif
